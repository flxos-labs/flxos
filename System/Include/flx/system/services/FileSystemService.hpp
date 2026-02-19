#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace flx::services {

struct FileEntry {
	std::string name;
	bool isDirectory {false};
	uint64_t size {0};
};

/// Progress callback: (percentComplete 0–100, currentItemPath)
using ProgressCallback = std::function<void(int, std::string_view)>;

class FileSystemService {
public:

	static FileSystemService& getInstance();

	// Non-copyable, non-movable singleton
	FileSystemService(const FileSystemService&) = delete;
	FileSystemService& operator=(const FileSystemService&) = delete;
	FileSystemService(FileSystemService&&) = delete;
	FileSystemService& operator=(FileSystemService&&) = delete;

	/// List entries in a directory. LVGL "A:/" paths are accepted.
	/// Returns an empty vector on failure (errors are logged internally).
	[[nodiscard]] std::vector<FileEntry> listDirectory(const std::string& path);

	/// Copy src → dst, recursively if src is a directory.
	[[nodiscard]] bool copy(const std::string& src, const std::string& dst, ProgressCallback callback = {});

	/// Move/rename src → dst (falls back to copy+delete across filesystems).
	[[nodiscard]] bool move(const std::string& src, const std::string& dst);

	/// Remove a file or directory tree.
	[[nodiscard]] bool remove(const std::string& path, ProgressCallback callback = {});

	/// Create a directory (succeeds if it already exists).
	[[nodiscard]] bool mkdir(const std::string& path);

	// ── Path helpers ──────────────────────────────────────────────────────────

	/// Strip LVGL drive prefix ("A:") → native FS path.
	[[nodiscard]] static std::string toNativePath(const std::string& lvPath);

	/// Join a base directory and a filename with exactly one '/'.
	[[nodiscard]] static std::string joinPath(std::string_view base, std::string_view name);

private:

	FileSystemService() = default;

	// Implementation helpers
	[[nodiscard]] static int copyFile(const char* src, const char* dst, int64_t totalBytes, ProgressCallback callback);

	[[nodiscard]] static int copyRecursive(const char* src, const char* dst, ProgressCallback callback);

	[[nodiscard]] static int removeRecursive(const char* path, ProgressCallback callback);
};

} // namespace flx::services