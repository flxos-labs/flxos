#pragma once

#include <cstdint>
#include <flx/system/services/FileOperationTypes.hpp>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace flx::kernel {
class Task;
}

namespace flx::services {

struct FileSystemPerfStats {
	uint64_t submitCount {0};
	uint64_t completedCount {0};
	uint64_t failedCount {0};
	uint64_t cancelledCount {0};
	uint64_t progressEventCount {0};
	uint64_t bytesRead {0};
	uint64_t bytesWritten {0};
	uint64_t maxChunkUs {0};
	uint64_t maxBusWaitUs {0};
	uint32_t queueDepth {0};
};

class FileSystemService {
public:

	static FileSystemService& getInstance();

	FileSystemService(const FileSystemService&) = delete;
	FileSystemService& operator=(const FileSystemService&) = delete;
	FileSystemService(FileSystemService&&) = delete;
	FileSystemService& operator=(FileSystemService&&) = delete;

	[[nodiscard]] FileOpId submit(
		FileOpRequest req,
		FileOpCallback onProgress = {},
		FileOpResultCallback onDone = {});

	[[nodiscard]] bool cancel(FileOpId opId);

	[[nodiscard]] FileOpState state(FileOpId opId) const;

	[[nodiscard]] FileSystemPerfStats getPerfStats() const;

	[[nodiscard]] static std::string toNativePath(const std::string& lvPath);

	[[nodiscard]] static std::string joinPath(std::string_view base, std::string_view name);

private:

	friend class FileOpExecutorTask;

	struct Job {
		FileOpRequest req {};
		FileOpCallback onProgress {};
		FileOpResultCallback onDone {};
	};

	struct OpRecord {
		FileOpType type {FileOpType::ListDirectory};
		FileOpState state {FileOpState::Queued};
		bool cancelRequested {false};
	};

	FileSystemService();
	~FileSystemService();

	void ensureExecutorStarted();
	void processJob(FileOpId opId);
	void updateQueueDepth();

	mutable std::mutex m_mutex {};
	std::unordered_map<FileOpId, OpRecord> m_records {};
	std::unordered_map<FileOpId, Job> m_jobs {};
	FileOpId m_nextId {1};

	void* m_queue {nullptr}; // QueueHandle_t
	flx::kernel::Task* m_executor {nullptr};

	FileSystemPerfStats m_perf {};
};

} // namespace flx::services
