#include <flx/system/services/FileSystemService.hpp>

#include "Config.hpp"
#include "sdkconfig.h"

#include <flx/core/Logger.hpp>
#include <flx/kernel/TaskManager.hpp>
#if FLXOS_SD_CARD_ENABLED
#include <flx/system/services/SdCardService.hpp>
#endif
#if !CONFIG_FLXOS_HEADLESS_MODE
#include "misc/lv_fs.h"
#include <flx/core/GuiLock.hpp>
#endif

#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <array>
#include <memory>
#include <utility>

// ─────────────────────────────────────────────────────────────────────────────
// Internal RAII helpers (file-local, zero overhead)
// ─────────────────────────────────────────────────────────────────────────────
namespace {

static constexpr std::string_view TAG = "FileSystemService";

// ── RAII: POSIX FILE* ────────────────────────────────────────────────────────
struct FileCloser {
	void operator()(FILE* f) const noexcept {
		if (f) std::fclose(f);
	}
};
using UniqueFile = std::unique_ptr<FILE, FileCloser>;

[[nodiscard]] UniqueFile openFile(const char* path, const char* mode) noexcept {
	return UniqueFile {std::fopen(path, mode)};
}

// ── RAII: POSIX DIR* ─────────────────────────────────────────────────────────
struct DirCloser {
	void operator()(DIR* d) const noexcept {
		if (d) ::closedir(d);
	}
};
using UniqueDir = std::unique_ptr<DIR, DirCloser>;

[[nodiscard]] UniqueDir openDir(const char* path) noexcept {
	return UniqueDir {::opendir(path)};
}

// ── RAII: LVGL lv_fs_dir_t (GUI builds only) ─────────────────────────────────
#if !CONFIG_FLXOS_HEADLESS_MODE
class LvDir {
public:

	[[nodiscard]] lv_fs_res_t open(const char* path) noexcept {
		lv_fs_res_t res = lv_fs_dir_open(&dir_, path);
		open_ = (res == LV_FS_RES_OK);
		return res;
	}

	[[nodiscard]] bool isOpen() const noexcept { return open_; }

	lv_fs_res_t read(char* buf, size_t bufSize) noexcept {
		return lv_fs_dir_read(&dir_, buf, static_cast<uint32_t>(bufSize));
	}

	~LvDir() noexcept {
		if (open_) lv_fs_dir_close(&dir_);
	}

	// Non-copyable
	LvDir(const LvDir&) = delete;
	LvDir& operator=(const LvDir&) = delete;

	LvDir() = default;

private:

	lv_fs_dir_t dir_ {};
	bool open_ {false};
};
#endif

// ── RAII: GuiLock (GUI builds only) ──────────────────────────────────────────
#if !CONFIG_FLXOS_HEADLESS_MODE
struct GuiLockGuard {
	GuiLockGuard() noexcept { flx::core::GuiLock::lock(); }
	~GuiLockGuard() noexcept { flx::core::GuiLock::unlock(); }

	GuiLockGuard(const GuiLockGuard&) = delete;
	GuiLockGuard& operator=(const GuiLockGuard&) = delete;
};
#endif

// ── Stat helpers ─────────────────────────────────────────────────────────────
[[nodiscard]] bool statPath(const char* path, struct stat& out) noexcept {
	return ::stat(path, &out) == 0;
}

[[nodiscard]] bool isDirectory(const struct stat& st) noexcept {
	return S_ISDIR(st.st_mode);
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// FileSystemService implementation
// ─────────────────────────────────────────────────────────────────────────────
namespace flx::services {

// ── Singleton ────────────────────────────────────────────────────────────────

FileSystemService& FileSystemService::getInstance() {
	static FileSystemService instance;
	return instance;
}

// ── Path utilities ────────────────────────────────────────────────────────────

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

// ── listDirectory ─────────────────────────────────────────────────────────────

std::vector<FileEntry> FileSystemService::listDirectory(const std::string& path) {
	std::vector<FileEntry> entries;

#if !CONFIG_FLXOS_HEADLESS_MODE
	// Hold the GUI lock for the full operation to prevent SPI bus contention.
	GuiLockGuard lock;

	// ── Special-case: LVGL virtual root "A:/" ────────────────────────────────
	const bool isLvRoot = (path == "A:/" || path == "A:");
	if (isLvRoot) {
		// Just synthesize the well-known sub-mounts directly.
		entries.push_back({"system", /*isDir=*/true, 0});
		entries.push_back({"data", /*isDir=*/true, 0});
#if FLXOS_SD_CARD_ENABLED
		if (SdCardService::getInstance().isMounted()) {
			entries.push_back({"sdcard", /*isDir=*/true, 0});
		}
#endif
		return entries;
	}
#endif

	const std::string nativePath = toNativePath(path);
	UniqueDir dir = openDir(nativePath.c_str());
	if (!dir) {
		Log::error(TAG, "Failed to open directory: %s", nativePath.c_str());
		return entries;
	}

	struct dirent* ent = nullptr;
	uint32_t count = 0;
	while ((ent = ::readdir(dir.get())) != nullptr) {
		const char* name = ent->d_name;
		if (std::strcmp(name, ".") == 0 || std::strcmp(name, "..") == 0) {
			continue;
		}

		// Feed watchdog every few entries to prevent reboot on slow SD cards
		if (++count % 16 == 0) {
			auto* guiTask = flx::kernel::TaskManager::getInstance().getTask("gui_task");
			if (guiTask) guiTask->heartbeat();
			vTaskDelay(pdMS_TO_TICKS(1));
		}

		FileEntry entry;
		entry.name = name;

		// Optimization: Use d_type if available to avoid slow stat() calls
		if (ent->d_type != DT_UNKNOWN) {
			entry.isDirectory = (ent->d_type == DT_DIR);
			entry.size = 0; // Size not needed for basic listing
		} else {
			const std::string fullPath = joinPath(nativePath, name);
			struct stat st {};
			if (statPath(fullPath.c_str(), st)) {
				entry.isDirectory = isDirectory(st);
				if (!entry.isDirectory) {
					entry.size = static_cast<uint64_t>(st.st_size);
				}
			} else {
				// Fallback if stat fails
				entry.isDirectory = (ent->d_type == DT_DIR);
				entry.size = 0;
			}
		}

		entries.push_back(std::move(entry));
	}

	Log::debug(TAG, "Listed %zu entries in '%s'", entries.size(), nativePath.c_str());
	return entries;
}

// ── copyFile ─────────────────────────────────────────────────────────────────

int FileSystemService::copyFile(const char* src, const char* dst, int64_t totalBytes, ProgressCallback callback) {
#if !CONFIG_FLXOS_HEADLESS_MODE
	GuiLockGuard lock;
#endif
	UniqueFile fsrc = openFile(src, "rb");
	if (!fsrc) {
		Log::error(TAG, "Cannot open source '%s': %s", src, std::strerror(errno));
		return -1;
	}

	UniqueFile fdst = openFile(dst, "wb");
	if (!fdst) {
		Log::error(TAG, "Cannot open destination '%s': %s", dst, std::strerror(errno));
		return -1;
	}

	Log::info(TAG, "Copying '%s' → '%s' (%" PRId64 " bytes)", src, dst, totalBytes);

	std::array<char, 4096> buf {};
	int64_t copied = 0;

	while (true) {
		const size_t n = std::fread(buf.data(), 1, buf.size(), fsrc.get());
		if (n == 0) {
			if (std::ferror(fsrc.get())) {
				Log::error(TAG, "Read error on '%s': %s", src, std::strerror(errno));
				return -1;
			}
			break; // EOF
		}

		if (std::fwrite(buf.data(), 1, n, fdst.get()) != n) {
			Log::error(TAG, "Write error on '%s': %s", dst, std::strerror(errno));
			return -1;
		}

		copied += static_cast<int64_t>(n);

		if (callback && totalBytes > 0) {
			const int pct = static_cast<int>(copied * 100 / totalBytes);
			callback(pct, src);
		}
	}

	// Flush to catch deferred write errors before closing.
	if (std::fflush(fdst.get()) != 0) {
		Log::error(TAG, "Flush error on '%s': %s", dst, std::strerror(errno));
		return -1;
	}

	Log::info(TAG, "Copy complete: '%s'", dst);
	return 0;
}

// ── copyRecursive ─────────────────────────────────────────────────────────────

int FileSystemService::copyRecursive(const char* src, const char* dst, ProgressCallback callback) {
#if !CONFIG_FLXOS_HEADLESS_MODE
	GuiLockGuard lock;
#endif
	struct stat st {};
	if (!statPath(src, st)) {
		Log::error(TAG, "Cannot stat '%s': %s", src, std::strerror(errno));
		return -1;
	}

	if (!isDirectory(st)) {
		const int64_t size = static_cast<int64_t>(st.st_size);
		return copyFile(src, dst, size, std::move(callback));
	}

	// ── Directory case ────────────────────────────────────────────────────────
	if (callback) callback(0, src);

	if (::mkdir(dst, 0777) != 0 && errno != EEXIST) {
		Log::error(TAG, "Cannot create directory '%s': %s", dst, std::strerror(errno));
		return -1;
	}

	UniqueDir dir = openDir(src);
	if (!dir) {
		Log::error(TAG, "Cannot open source directory '%s': %s", src, std::strerror(errno));
		return -1;
	}

	while (const struct dirent* entry = ::readdir(dir.get())) {
		if (std::strcmp(entry->d_name, ".") == 0 ||
			std::strcmp(entry->d_name, "..") == 0) {
			continue;
		}

		const std::string subSrc = joinPath(src, entry->d_name);
		const std::string subDst = joinPath(dst, entry->d_name);

		if (copyRecursive(subSrc.c_str(), subDst.c_str(), callback) != 0) {
			return -1;
		}
	}

	return 0;
}

// ── copy (public) ─────────────────────────────────────────────────────────────

bool FileSystemService::copy(const std::string& src, const std::string& dst, ProgressCallback callback) {
	return copyRecursive(src.c_str(), dst.c_str(), std::move(callback)) == 0;
}

// ── move ──────────────────────────────────────────────────────────────────────

bool FileSystemService::move(const std::string& src, const std::string& dst) {
#if !CONFIG_FLXOS_HEADLESS_MODE
	GuiLockGuard lock;
#endif
	// Try atomic rename first (works within the same filesystem).
	if (::rename(src.c_str(), dst.c_str()) == 0) {
		Log::info(TAG, "Moved '%s' → '%s'", src.c_str(), dst.c_str());
		return true;
	}

	// Cross-filesystem move: copy then delete.
	if (errno == EXDEV) {
		Log::info(TAG, "Cross-device move detected; falling back to copy+delete");
		if (!copy(src, dst)) {
			Log::error(TAG, "Cross-device copy failed: '%s' → '%s'", src.c_str(), dst.c_str());
			return false;
		}
		if (!remove(src)) {
			Log::warn(TAG, "Copy succeeded but failed to remove source '%s'", src.c_str());
			// Still report success since data reached the destination.
		}
		return true;
	}

	Log::error(TAG, "rename('%s', '%s') failed: %s", src.c_str(), dst.c_str(), std::strerror(errno));
	return false;
}

// ── removeRecursive ───────────────────────────────────────────────────────────

int FileSystemService::removeRecursive(const char* path, ProgressCallback callback) {
#if !CONFIG_FLXOS_HEADLESS_MODE
	GuiLockGuard lock;
#endif
	UniqueDir dir = openDir(path);
	if (!dir) {
		// Not a directory (or can't be opened) — attempt plain file removal.
		if (::unlink(path) != 0) {
			Log::error(TAG, "unlink('%s') failed: %s", path, std::strerror(errno));
			return -1;
		}
		return 0;
	}

	while (const struct dirent* entry = ::readdir(dir.get())) {
		if (std::strcmp(entry->d_name, ".") == 0 ||
			std::strcmp(entry->d_name, "..") == 0) {
			continue;
		}

		const std::string subPath = joinPath(path, entry->d_name);
		if (callback) callback(0, subPath);

		struct stat st {};
		if (!statPath(subPath.c_str(), st)) {
			Log::error(TAG, "Cannot stat '%s': %s", subPath.c_str(), std::strerror(errno));
			return -1;
		}

		const int r = isDirectory(st)
			? removeRecursive(subPath.c_str(), callback)
			: ::unlink(subPath.c_str());

		if (r != 0) {
			Log::error(TAG, "Failed to remove '%s': %s", subPath.c_str(), std::strerror(errno));
			return -1;
		}
	}

	dir.reset(); // Close before rmdir — required on some FSes.

	if (::rmdir(path) != 0) {
		Log::error(TAG, "rmdir('%s') failed: %s", path, std::strerror(errno));
		return -1;
	}

	return 0;
}

// ── remove (public) ───────────────────────────────────────────────────────────

bool FileSystemService::remove(const std::string& path, ProgressCallback callback) {
#if !CONFIG_FLXOS_HEADLESS_MODE
	GuiLockGuard lock;
#endif
	struct stat st {};
	if (statPath(path.c_str(), st) && isDirectory(st)) {
		Log::info(TAG, "Removing directory tree: %s", path.c_str());
		return removeRecursive(path.c_str(), std::move(callback)) == 0;
	}

	Log::info(TAG, "Removing file: %s", path.c_str());
	if (::unlink(path.c_str()) != 0) {
		Log::error(TAG, "unlink('%s') failed: %s", path.c_str(), std::strerror(errno));
		return false;
	}
	return true;
}

// ── mkdir ─────────────────────────────────────────────────────────────────────

bool FileSystemService::mkdir(const std::string& path) {
#if !CONFIG_FLXOS_HEADLESS_MODE
	GuiLockGuard lock;
#endif
	if (::mkdir(path.c_str(), 0777) == 0) {
		Log::info(TAG, "Created directory: %s", path.c_str());
		return true;
	}

	if (errno == EEXIST) {
		// Verify it's actually a directory, not a file with the same name.
		struct stat st {};
		if (statPath(path.c_str(), st) && isDirectory(st)) {
			return true;
		}
		Log::error(TAG, "Path exists but is not a directory: %s", path.c_str());
		return false;
	}

	Log::error(TAG, "mkdir('%s') failed: %s", path.c_str(), std::strerror(errno));
	return false;
}

} // namespace flx::services
