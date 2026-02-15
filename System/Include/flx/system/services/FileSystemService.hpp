#pragma once

#include <functional>
#include <string>
#include <vector>

namespace flx::services {

struct FileEntry {
	std::string name {};
	bool isDirectory {};
	size_t size {}; // For files, 0 for directories
};

using ProgressCallback = std::function<void(int percent, const char* currentPath)>;

class FileSystemService {
public:

	static FileSystemService& getInstance();

	/**
	 * List all files and directories in the given path.
	 * @param path The directory path (LVGL format, e.g., "A:/data")
	 * @return Vector of FileEntry objects
	 */
	std::vector<FileEntry> listDirectory(const std::string& path);

	/**
	 * Copy a file or directory recursively.
	 * @param src Source path (VFS format, e.g., "/data/file.txt")
	 * @param dst Destination path (VFS format)
	 * @param callback Optional progress callback
	 * @return true on success, false on failure
	 */
	bool copy(const std::string& src, const std::string& dst, ProgressCallback callback = nullptr);

	/**
	 * Move/rename a file or directory.
	 * @param src Source path (VFS format)
	 * @param dst Destination path (VFS format)
	 * @return true on success, false on failure
	 */
	bool move(const std::string& src, const std::string& dst);

	/**
	 * Remove a file or directory recursively.
	 * @param path Path to remove (VFS format)
	 * @param callback Optional progress callback
	 * @return true on success, false on failure
	 */
	bool remove(const std::string& path, ProgressCallback callback = nullptr);

	/**
	 * Create a new directory.
	 * @param path Directory path (VFS format)
	 * @return true on success, false on failure
	 */
	bool mkdir(const std::string& path);

	/**
	 * Convert LVGL path (A:/) to VFS path (/)
	 * @param lvPath LVGL-style path
	 * @return VFS-style path
	 */
	static std::string toVfsPath(const std::string& lvPath);

	/**
	 * Build a full path from base and name, handling slashes properly.
	 * @param base Base directory path
	 * @param name File or directory name
	 * @return Combined path
	 */
	static std::string buildPath(const std::string& base, const std::string& name);

private:

	FileSystemService() = default;
	~FileSystemService() = default;
	FileSystemService(const FileSystemService&) = delete;
	FileSystemService& operator=(const FileSystemService&) = delete;

	// Internal helper methods
	int copyFile(const char* src, const char* dst, ProgressCallback callback);
	int copyRecursive(const char* src, const char* dst, ProgressCallback callback);
	int removeRecursive(const char* path, ProgressCallback callback);
};

} // namespace flx::services
