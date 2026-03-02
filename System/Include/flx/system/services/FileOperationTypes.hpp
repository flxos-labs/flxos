#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace flx::services {

using FileOpId = uint32_t;

struct FileEntry {
	std::string name;
	bool isDirectory {false};
	uint64_t size {0};
};

enum class FileOpType {
	ListDirectory,
	Copy,
	Move,
	Remove,
	Mkdir
};

enum class FileOpState {
	Queued,
	Running,
	Completed,
	Failed,
	Cancelled
};

struct FileOpRequest {
	FileOpType type {FileOpType::ListDirectory};
	std::string path {};
	std::string destinationPath {};
};

struct FileOpProgress {
	FileOpId opId {0};
	FileOpType type {FileOpType::ListDirectory};
	FileOpState state {FileOpState::Queued};
	std::string currentPath {};
	int percent {-1}; // -1 when unknown
	uint64_t bytesDone {0};
	uint64_t bytesTotal {0};
	int errorCode {0};
};

struct FileOpResult {
	FileOpId opId {0};
	FileOpType type {FileOpType::ListDirectory};
	FileOpState state {FileOpState::Failed};
	bool success {false};
	int errorCode {0};
	std::string errorMessage {};
	std::vector<FileEntry> entries {};
};

using FileOpCallback = std::function<void(const FileOpProgress&)>;
using FileOpResultCallback = std::function<void(const FileOpResult&)>;

} // namespace flx::services
