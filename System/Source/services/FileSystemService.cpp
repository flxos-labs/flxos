#include <flx/system/services/FileSystemService.hpp>

#include "Config.hpp"
#include "esp_timer.h"
#include "sdkconfig.h"

#include <flx/core/Logger.hpp>
#include <flx/hal/BusManager.hpp>
#include <flx/kernel/Task.hpp>
#if FLXOS_SD_CARD_ENABLED
#include <flx/system/services/SdCardService.hpp>
#endif

#include "freertos/projdefs.h"
#include "freertos/queue.h"

#include <array>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <functional>
#include <memory>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

namespace {

static constexpr std::string_view TAG = "FileSystemService";
constexpr uint32_t kQueueDepth = 8;
constexpr uint32_t kExecutorStack = 14 * 1024;
constexpr uint32_t kExecutorPriority = 4;

struct FileCloser {
	void operator()(FILE* f) const noexcept {
		if (f) std::fclose(f);
	}
};
using UniqueFile = std::unique_ptr<FILE, FileCloser>;

[[nodiscard]] UniqueFile openFile(const char* path, const char* mode) noexcept {
	return UniqueFile {std::fopen(path, mode)};
}

struct DirCloser {
	void operator()(DIR* d) const noexcept {
		if (d) ::closedir(d);
	}
};
using UniqueDir = std::unique_ptr<DIR, DirCloser>;

[[nodiscard]] UniqueDir openDir(const char* path) noexcept {
	return UniqueDir {::opendir(path)};
}

[[nodiscard]] bool isDirectory(const struct stat& st) noexcept {
	return S_ISDIR(st.st_mode);
}

[[nodiscard]] bool isLvRootPath(std::string_view path) noexcept {
	return path == "A:/" || path == "A:";
}

[[nodiscard]] bool isVirtualRootPath(std::string_view path) noexcept {
	return isLvRootPath(path) || path == "/";
}

[[maybe_unused]] [[nodiscard]] bool isUnderMount(std::string_view path, std::string_view mount) {
	if (mount.empty()) return false;
	if (path.size() < mount.size()) return false;
	if (path.compare(0, mount.size(), mount) != 0) return false;
	if (path.size() == mount.size()) return true;
	return path[mount.size()] == '/';
}

[[nodiscard]] bool isSdPathNative(std::string_view nativePath) {
#if FLXOS_SD_CARD_ENABLED
	return isUnderMount(nativePath, flx::config::sdcard.mountPoint);
#else
	(void)nativePath;
	return false;
#endif
}

template<typename T, typename Fn>
T withSpiLockValue(bool useSpi, uint64_t& maxBusWaitUs, T failValue, Fn&& fn) {
	if (!useSpi) {
		return fn();
	}

	uint64_t const waitStartUs = static_cast<uint64_t>(esp_timer_get_time());
	flx::hal::BusManager::ScopedBusLock busLock(flx::config::sdcard.spiHost);
	uint64_t const waitedUs = static_cast<uint64_t>(esp_timer_get_time()) - waitStartUs;
	if (waitedUs > maxBusWaitUs) {
		maxBusWaitUs = waitedUs;
	}

	if (!busLock.isAcquired()) {
		errno = ETIMEDOUT;
		return failValue;
	}

	return fn();
}

std::string errnoToString(int err) {
	return std::strerror(err);
}

} // namespace

namespace flx::services {

class FileOpExecutorTask final : public flx::kernel::Task {
public:

	explicit FileOpExecutorTask(FileSystemService& service)
		: flx::kernel::Task("fs_op_executor", kExecutorStack, kExecutorPriority, 0),
		  m_service(service) {
		setRestartPolicy(RestartPolicy::RESTART_TASK);
	}

protected:

	void run(void* /*data*/) override {
		setWatchdogTimeout(10000);
		auto queue = static_cast<QueueHandle_t>(m_service.m_queue);
		if (!queue) {
			Log::error(TAG, "File operation queue is null");
			vTaskDelete(nullptr);
			return;
		}

		while (true) {
			heartbeat();
			FileOpId opId = 0;
			if (xQueueReceive(queue, &opId, pdMS_TO_TICKS(100)) == pdTRUE) {
				m_service.processJob(opId);
				m_service.updateQueueDepth();
			}
		}
	}

private:

	FileSystemService& m_service;
};

FileSystemService& FileSystemService::getInstance() {
	static FileSystemService instance;
	return instance;
}

FileSystemService::FileSystemService() {
	m_queue = xQueueCreate(kQueueDepth, sizeof(FileOpId));
	if (!m_queue) {
		Log::error(TAG, "Failed to create file operation queue");
		return;
	}
	ensureExecutorStarted();
	updateQueueDepth();
}

FileSystemService::~FileSystemService() {
	if (m_executor) {
		m_executor->stop();
		delete m_executor;
		m_executor = nullptr;
	}
	if (m_queue) {
		vQueueDelete(static_cast<QueueHandle_t>(m_queue));
		m_queue = nullptr;
	}
}

std::string FileSystemService::toNativePath(const std::string& lvPath) {
	constexpr std::string_view lvPrefix = "A:";
	if (lvPath.size() >= lvPrefix.size() &&
		lvPath.compare(0, lvPrefix.size(), lvPrefix) == 0) {
		return lvPath.substr(lvPrefix.size());
	}
	return lvPath;
}

std::string FileSystemService::joinPath(std::string_view base, std::string_view name) {
	if (base.empty()) return std::string {name};
	if (base.back() == '/') {
		return std::string {base} + std::string {name};
	}
	return std::string {base} + '/' + std::string {name};
}

void FileSystemService::ensureExecutorStarted() {
	if (m_executor || !m_queue) return;

	auto* task = new FileOpExecutorTask(*this);
	if (!task->start()) {
		Log::error(TAG, "Failed to start file operation executor task");
		delete task;
		return;
	}
	m_executor = task;
}

FileOpId FileSystemService::submit(FileOpRequest req, FileOpCallback onProgress, FileOpResultCallback onDone) {
	ensureExecutorStarted();
	auto queue = static_cast<QueueHandle_t>(m_queue);
	if (!queue) {
		if (onDone) {
			FileOpResult result;
			result.opId = 0;
			result.type = req.type;
			result.state = FileOpState::Failed;
			result.success = false;
			result.errorCode = ENODEV;
			result.errorMessage = "file operation queue unavailable";
			onDone(result);
		}
		return 0;
	}

	FileOpId opId = 0;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		opId = m_nextId++;
		m_jobs.emplace(opId, Job {std::move(req), std::move(onProgress), std::move(onDone)});
		m_records.emplace(opId, OpRecord {m_jobs[opId].req.type, FileOpState::Queued, false});
		++m_perf.submitCount;
	}

	if (xQueueSend(queue, &opId, 0) != pdTRUE) {
		Job failedJob;
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			auto recIt = m_records.find(opId);
			if (recIt != m_records.end()) {
				recIt->second.state = FileOpState::Failed;
			}
			auto jobIt = m_jobs.find(opId);
			if (jobIt != m_jobs.end()) {
				failedJob = std::move(jobIt->second);
				m_jobs.erase(jobIt);
			}
			++m_perf.failedCount;
			updateQueueDepth();
		}

		if (failedJob.onDone) {
			FileOpResult result;
			result.opId = opId;
			result.type = failedJob.req.type;
			result.state = FileOpState::Failed;
			result.success = false;
			result.errorCode = EBUSY;
			result.errorMessage = "file operation queue full";
			failedJob.onDone(result);
		}
		return opId;
	}

	updateQueueDepth();
	return opId;
}

bool FileSystemService::cancel(FileOpId opId) {
	std::lock_guard<std::mutex> lock(m_mutex);
	auto it = m_records.find(opId);
	if (it == m_records.end()) return false;
	if (it->second.state == FileOpState::Completed ||
		it->second.state == FileOpState::Failed ||
		it->second.state == FileOpState::Cancelled) {
		return false;
	}
	it->second.cancelRequested = true;
	return true;
}

FileOpState FileSystemService::state(FileOpId opId) const {
	std::lock_guard<std::mutex> lock(m_mutex);
	auto it = m_records.find(opId);
	if (it == m_records.end()) return FileOpState::Failed;
	return it->second.state;
}

FileSystemPerfStats FileSystemService::getPerfStats() const {
	std::lock_guard<std::mutex> lock(m_mutex);
	FileSystemPerfStats out = m_perf;
	if (m_queue) {
		out.queueDepth = uxQueueMessagesWaiting(static_cast<QueueHandle_t>(m_queue));
	}
	return out;
}

void FileSystemService::updateQueueDepth() {
	if (!m_queue) {
		m_perf.queueDepth = 0;
		return;
	}
	m_perf.queueDepth = uxQueueMessagesWaiting(static_cast<QueueHandle_t>(m_queue));
}

void FileSystemService::processJob(FileOpId opId) {
	Job job;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		auto recIt = m_records.find(opId);
		auto jobIt = m_jobs.find(opId);
		if (recIt == m_records.end() || jobIt == m_jobs.end()) {
			return;
		}
		recIt->second.state = FileOpState::Running;
		job = jobIt->second;
	}

	auto isCancelled = [this, opId]() -> bool {
		std::lock_guard<std::mutex> lock(m_mutex);
		auto it = m_records.find(opId);
		return (it != m_records.end() && it->second.cancelRequested);
	};

	uint64_t localBytesRead = 0;
	uint64_t localBytesWritten = 0;
	uint64_t localMaxChunkUs = 0;
	uint64_t localMaxBusWaitUs = 0;

	auto emitProgress = [&](FileOpState stateValue, const std::string& path, int percent, uint64_t done, uint64_t total, int errorCode) {
		if (!job.onProgress) return;
		FileOpProgress p;
		p.opId = opId;
		p.type = job.req.type;
		p.state = stateValue;
		p.currentPath = path;
		p.percent = percent;
		p.bytesDone = done;
		p.bytesTotal = total;
		p.errorCode = errorCode;
		job.onProgress(p);
		std::lock_guard<std::mutex> lock(m_mutex);
		++m_perf.progressEventCount;
	};

	FileOpResult result;
	result.opId = opId;
	result.type = job.req.type;
	result.state = FileOpState::Failed;
	result.success = false;

	if (isCancelled()) {
		result.state = FileOpState::Cancelled;
		result.errorCode = ECANCELED;
		result.errorMessage = "operation cancelled";
	} else {
		switch (job.req.type) {
			case FileOpType::ListDirectory: {
				std::vector<FileEntry> entries;

				if (isVirtualRootPath(job.req.path)) {
					entries.push_back({"system", true, 0});
					entries.push_back({"data", true, 0});
#if FLXOS_SD_CARD_ENABLED
					if (SdCardService::getInstance().isMounted()) {
						entries.push_back({"sdcard", true, 0});
					}
#endif
					result.entries = std::move(entries);
					result.state = FileOpState::Completed;
					result.success = true;
					break;
				}

				std::string const nativePath = toNativePath(job.req.path);
				bool const useSpi = isSdPathNative(nativePath);
				UniqueDir dir = withSpiLockValue(useSpi, localMaxBusWaitUs, UniqueDir {}, [&]() {
					return openDir(nativePath.c_str());
				});
				if (!dir) {
					result.errorCode = errno;
					result.errorMessage = errnoToString(result.errorCode);
					break;
				}

				while (true) {
					if (isCancelled()) {
						result.state = FileOpState::Cancelled;
						result.errorCode = ECANCELED;
						result.errorMessage = "operation cancelled";
						break;
					}

					errno = 0;
					struct dirent* ent = withSpiLockValue(useSpi, localMaxBusWaitUs, static_cast<struct dirent*>(nullptr), [&]() {
						return ::readdir(dir.get());
					});
					if (!ent) {
						if (errno != 0) {
							result.errorCode = errno;
							result.errorMessage = errnoToString(errno);
						}
						break;
					}

					if (std::strcmp(ent->d_name, ".") == 0 || std::strcmp(ent->d_name, "..") == 0) {
						continue;
					}

					FileEntry entry;
					entry.name = ent->d_name;
					if (ent->d_type != DT_UNKNOWN) {
						entry.isDirectory = (ent->d_type == DT_DIR);
					} else {
						std::string const fullPath = joinPath(nativePath, ent->d_name);
						struct stat st {};
						int const stRes = withSpiLockValue(useSpi, localMaxBusWaitUs, -1, [&]() {
							return ::stat(fullPath.c_str(), &st);
						});
						if (stRes == 0) {
							entry.isDirectory = isDirectory(st);
							if (!entry.isDirectory) {
								entry.size = static_cast<uint64_t>(st.st_size);
							}
						} else {
							entry.isDirectory = false;
						}
					}
					entries.push_back(std::move(entry));
				}

				if (result.state != FileOpState::Cancelled && result.errorCode == 0) {
					result.entries = std::move(entries);
					result.state = FileOpState::Completed;
					result.success = true;
				}
				break;
			}

			case FileOpType::Mkdir: {
				std::string const path = toNativePath(job.req.path);
				bool const useSpi = isSdPathNative(path);
				int const mkRes = withSpiLockValue(useSpi, localMaxBusWaitUs, -1, [&]() {
					return ::mkdir(path.c_str(), 0777);
				});
				if (mkRes == 0) {
					result.state = FileOpState::Completed;
					result.success = true;
					break;
				}

				if (errno == EEXIST) {
					struct stat st {};
					int const stRes = withSpiLockValue(useSpi, localMaxBusWaitUs, -1, [&]() {
						return ::stat(path.c_str(), &st);
					});
					if (stRes == 0 && isDirectory(st)) {
						result.state = FileOpState::Completed;
						result.success = true;
						break;
					}
				}

				result.errorCode = errno;
				result.errorMessage = errnoToString(errno);
				break;
			}

			case FileOpType::Copy:
			case FileOpType::Move: {
				std::string const src = toNativePath(job.req.path);
				std::string const dst = toNativePath(job.req.destinationPath);

				auto copyFile = [&](const std::string& fileSrc, const std::string& fileDst) -> int {
					bool const useSpi = isSdPathNative(fileSrc) || isSdPathNative(fileDst);

					UniqueFile fsrc = withSpiLockValue(useSpi, localMaxBusWaitUs, UniqueFile {}, [&]() {
						return openFile(fileSrc.c_str(), "rb");
					});
					if (!fsrc) return -1;

					UniqueFile fdst = withSpiLockValue(useSpi, localMaxBusWaitUs, UniqueFile {}, [&]() {
						return openFile(fileDst.c_str(), "wb");
					});
					if (!fdst) return -1;

					struct stat st {};
					uint64_t totalBytes = 0;
					if (withSpiLockValue(useSpi, localMaxBusWaitUs, -1, [&]() {
							return ::stat(fileSrc.c_str(), &st);
						}) == 0) {
						totalBytes = static_cast<uint64_t>(st.st_size);
					}

					std::array<char, 4096> buf {};
					uint64_t copied = 0;
					while (true) {
						if (isCancelled()) {
							withSpiLockValue(useSpi, localMaxBusWaitUs, -1, [&]() {
								return ::unlink(fileDst.c_str());
							});
							return -2;
						}

						uint64_t const chunkStartUs = static_cast<uint64_t>(esp_timer_get_time());

						errno = 0;
						size_t const n = withSpiLockValue(useSpi, localMaxBusWaitUs, static_cast<size_t>(0), [&]() {
							return std::fread(buf.data(), 1, buf.size(), fsrc.get());
						});
						if (n == 0) {
							if (errno == ETIMEDOUT) return -1;
							bool const hasError = withSpiLockValue(useSpi, localMaxBusWaitUs, false, [&]() {
								return std::ferror(fsrc.get()) != 0;
							});
							if (hasError) return -1;
							break;
						}

						size_t const w = withSpiLockValue(useSpi, localMaxBusWaitUs, static_cast<size_t>(0), [&]() {
							return std::fwrite(buf.data(), 1, n, fdst.get());
						});
						if (w != n) return -1;

						copied += static_cast<uint64_t>(n);
						localBytesRead += static_cast<uint64_t>(n);
						localBytesWritten += static_cast<uint64_t>(n);

						if (totalBytes > 0) {
							int const pct = static_cast<int>((copied * 100ULL) / totalBytes);
							emitProgress(FileOpState::Running, fileSrc, pct, copied, totalBytes, 0);
						}

						uint64_t const chunkUs = static_cast<uint64_t>(esp_timer_get_time()) - chunkStartUs;
						if (chunkUs > localMaxChunkUs) {
							localMaxChunkUs = chunkUs;
						}
					}

					int const flushRes = withSpiLockValue(useSpi, localMaxBusWaitUs, EOF, [&]() {
						return std::fflush(fdst.get());
					});
					if (flushRes != 0) return -1;
					return 0;
				};

				std::function<int(const std::string&, const std::string&)> copyRecursive;
				copyRecursive = [&](const std::string& treeSrc, const std::string& treeDst) -> int {
					if (isCancelled()) return -2;

					bool const useSpi = isSdPathNative(treeSrc) || isSdPathNative(treeDst);
					struct stat st {};
					int const stRes = withSpiLockValue(useSpi, localMaxBusWaitUs, -1, [&]() {
						return ::stat(treeSrc.c_str(), &st);
					});
					if (stRes != 0) return -1;

					if (!isDirectory(st)) {
						return copyFile(treeSrc, treeDst);
					}

					emitProgress(FileOpState::Running, treeSrc, 0, 0, 0, 0);

					int const mkRes = withSpiLockValue(useSpi, localMaxBusWaitUs, -1, [&]() {
						return ::mkdir(treeDst.c_str(), 0777);
					});
					if (mkRes != 0 && errno != EEXIST) return -1;

					UniqueDir dir = withSpiLockValue(useSpi, localMaxBusWaitUs, UniqueDir {}, [&]() {
						return openDir(treeSrc.c_str());
					});
					if (!dir) return -1;

					while (true) {
						if (isCancelled()) return -2;
						errno = 0;
						struct dirent* entry = withSpiLockValue(useSpi, localMaxBusWaitUs, static_cast<struct dirent*>(nullptr), [&]() {
							return ::readdir(dir.get());
						});
						if (!entry) {
							if (errno != 0) return -1;
							break;
						}
						if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0) {
							continue;
						}

						std::string const subSrc = joinPath(treeSrc, entry->d_name);
						std::string const subDst = joinPath(treeDst, entry->d_name);
						int const r = copyRecursive(subSrc, subDst);
						if (r != 0) return r;
					}
					return 0;
				};

				std::function<int(const std::string&)> removeRecursive;
				removeRecursive = [&](const std::string& targetPath) -> int {
					if (isCancelled()) return -2;

					bool const useSpi = isSdPathNative(targetPath);
					UniqueDir dir = withSpiLockValue(useSpi, localMaxBusWaitUs, UniqueDir {}, [&]() {
						return openDir(targetPath.c_str());
					});
					if (!dir) {
						int const u = withSpiLockValue(useSpi, localMaxBusWaitUs, -1, [&]() {
							return ::unlink(targetPath.c_str());
						});
						return (u == 0) ? 0 : -1;
					}

					while (true) {
						if (isCancelled()) return -2;
						errno = 0;
						struct dirent* entry = withSpiLockValue(useSpi, localMaxBusWaitUs, static_cast<struct dirent*>(nullptr), [&]() {
							return ::readdir(dir.get());
						});
						if (!entry) {
							if (errno != 0) return -1;
							break;
						}
						if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0) {
							continue;
						}

						std::string const subPath = joinPath(targetPath, entry->d_name);
						struct stat st {};
						int const stRes = withSpiLockValue(isSdPathNative(subPath), localMaxBusWaitUs, -1, [&]() {
							return ::stat(subPath.c_str(), &st);
						});
						if (stRes != 0) return -1;

						int r = 0;
						if (isDirectory(st)) {
							r = removeRecursive(subPath);
						} else {
							r = withSpiLockValue(isSdPathNative(subPath), localMaxBusWaitUs, -1, [&]() {
								return ::unlink(subPath.c_str());
							});
						}
						if (r != 0) return r;
					}
					dir.reset();
					int const rd = withSpiLockValue(useSpi, localMaxBusWaitUs, -1, [&]() {
						return ::rmdir(targetPath.c_str());
					});
					return (rd == 0) ? 0 : -1;
				};

				if (job.req.type == FileOpType::Move) {
					bool const renameUseSpi = isSdPathNative(src) || isSdPathNative(dst);
					int const renameRes = withSpiLockValue(renameUseSpi, localMaxBusWaitUs, -1, [&]() {
						return ::rename(src.c_str(), dst.c_str());
					});
					if (renameRes == 0) {
						result.state = FileOpState::Completed;
						result.success = true;
						break;
					}
					if (errno != EXDEV) {
						result.errorCode = errno;
						result.errorMessage = errnoToString(errno);
						break;
					}
				}

				int const copyRes = copyRecursive(src, dst);
				if (copyRes == -2) {
					result.state = FileOpState::Cancelled;
					result.errorCode = ECANCELED;
					result.errorMessage = "operation cancelled";
					break;
				}
				if (copyRes != 0) {
					result.errorCode = errno;
					result.errorMessage = errnoToString(result.errorCode);
					break;
				}

				if (job.req.type == FileOpType::Move) {
					int const rmRes = removeRecursive(src);
					if (rmRes == -2) {
						result.state = FileOpState::Cancelled;
						result.errorCode = ECANCELED;
						result.errorMessage = "move cancelled during source removal";
						break;
					}
					if (rmRes != 0) {
						Log::warn(TAG, "Move copied destination but failed to remove source '%s'", src.c_str());
					}
				}

				result.state = FileOpState::Completed;
				result.success = true;
				break;
			}

			case FileOpType::Remove: {
				std::string const path = toNativePath(job.req.path);

				std::function<int(const std::string&)> removeRecursive;
				removeRecursive = [&](const std::string& targetPath) -> int {
					if (isCancelled()) return -2;

					bool const useSpi = isSdPathNative(targetPath);
					UniqueDir dir = withSpiLockValue(useSpi, localMaxBusWaitUs, UniqueDir {}, [&]() {
						return openDir(targetPath.c_str());
					});
					if (!dir) {
						int const u = withSpiLockValue(useSpi, localMaxBusWaitUs, -1, [&]() {
							return ::unlink(targetPath.c_str());
						});
						return (u == 0) ? 0 : -1;
					}

					while (true) {
						if (isCancelled()) return -2;
						errno = 0;
						struct dirent* entry = withSpiLockValue(useSpi, localMaxBusWaitUs, static_cast<struct dirent*>(nullptr), [&]() {
							return ::readdir(dir.get());
						});
						if (!entry) {
							if (errno != 0) return -1;
							break;
						}
						if (std::strcmp(entry->d_name, ".") == 0 || std::strcmp(entry->d_name, "..") == 0) {
							continue;
						}

						std::string const subPath = joinPath(targetPath, entry->d_name);
						emitProgress(FileOpState::Running, subPath, -1, 0, 0, 0);

						struct stat st {};
						int const stRes = withSpiLockValue(isSdPathNative(subPath), localMaxBusWaitUs, -1, [&]() {
							return ::stat(subPath.c_str(), &st);
						});
						if (stRes != 0) return -1;

						int r = 0;
						if (isDirectory(st)) {
							r = removeRecursive(subPath);
						} else {
							r = withSpiLockValue(isSdPathNative(subPath), localMaxBusWaitUs, -1, [&]() {
								return ::unlink(subPath.c_str());
							});
						}
						if (r != 0) return r;
					}

					dir.reset();
					int const rr = withSpiLockValue(useSpi, localMaxBusWaitUs, -1, [&]() {
						return ::rmdir(targetPath.c_str());
					});
					return (rr == 0) ? 0 : -1;
				};

				struct stat st {};
				int const stRes = withSpiLockValue(isSdPathNative(path), localMaxBusWaitUs, -1, [&]() {
					return ::stat(path.c_str(), &st);
				});

				int rmRes = -1;
				if (stRes == 0 && isDirectory(st)) {
					rmRes = removeRecursive(path);
				} else {
					rmRes = withSpiLockValue(isSdPathNative(path), localMaxBusWaitUs, -1, [&]() {
						return ::unlink(path.c_str());
					});
				}

				if (rmRes == -2) {
					result.state = FileOpState::Cancelled;
					result.errorCode = ECANCELED;
					result.errorMessage = "operation cancelled";
					break;
				}
				if (rmRes != 0) {
					result.errorCode = errno;
					result.errorMessage = errnoToString(result.errorCode);
					break;
				}

				result.state = FileOpState::Completed;
				result.success = true;
				break;
			}
		}
	}

	if (result.success && result.state != FileOpState::Completed) {
		result.state = FileOpState::Completed;
	}
	if (!result.success &&
		result.state != FileOpState::Failed &&
		result.state != FileOpState::Cancelled) {
		result.state = FileOpState::Failed;
	}

	{
		std::lock_guard<std::mutex> lock(m_mutex);
		auto recIt = m_records.find(opId);
		if (recIt != m_records.end()) {
			recIt->second.state = result.state;
		}

		if (result.state == FileOpState::Completed) {
			++m_perf.completedCount;
		} else if (result.state == FileOpState::Cancelled) {
			++m_perf.cancelledCount;
		} else {
			++m_perf.failedCount;
		}

		m_perf.bytesRead += localBytesRead;
		m_perf.bytesWritten += localBytesWritten;
		if (localMaxChunkUs > m_perf.maxChunkUs) {
			m_perf.maxChunkUs = localMaxChunkUs;
		}
		if (localMaxBusWaitUs > m_perf.maxBusWaitUs) {
			m_perf.maxBusWaitUs = localMaxBusWaitUs;
		}

		m_jobs.erase(opId);
	}

	if (job.onDone) {
		job.onDone(result);
	}
}

} // namespace flx::services
