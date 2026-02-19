#include "Config.hpp"
#include "misc/lv_fs.h"
#include "sdkconfig.h"
#include <flx/core/GuiLock.hpp>
#include <flx/core/Logger.hpp>
#include <flx/system/services/FileSystemService.hpp>
#if defined(FLXOS_SD_CARD_ENABLED)
#include <flx/system/services/SdCardService.hpp>
#endif
#include <cstring>
#include <dirent.h>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>

static constexpr std::string_view TAG = "FileSystem";

namespace flx::services {

FileSystemService& FileSystemService::getInstance() {
	static FileSystemService instance;
	return instance;
}

std::string FileSystemService::toVfsPath(const std::string& lvPath) {
	if (lvPath.substr(0, 2) == "A:") {
		return lvPath.substr(2);
	}
	return lvPath;
}

std::string FileSystemService::buildPath(const std::string& base, const std::string& name) {
	return (base.back() == '/') ? base + name : base + "/" + name;
}

std::vector<FileEntry> FileSystemService::listDirectory(const std::string& path) {
	std::vector<FileEntry> entries;

	// Synchronize with GUI task to prevent SPI bus contention if SD is on shared bus
	flx::core::GuiLock::lock();

	// Special handling for root "A:/" which may not exist as a real directory
	if (path == "A:/" || path == "A:") {
		// Check if actual directories exist, otherwise return defaults
		lv_fs_dir_t rootDir;
		lv_fs_res_t rootRes = lv_fs_dir_open(&rootDir, path.c_str());

		if (rootRes != LV_FS_RES_OK) {
			Log::warn(TAG, "Failed to open root A:/, returning defaults");
			flx::core::GuiLock::unlock();
			// Return default directories
			entries.push_back({"system", true, 0});
			entries.push_back({"data", true, 0});
#if defined(FLXOS_SD_CARD_ENABLED)
			if (SdCardService::getInstance().isMounted()) {
				entries.push_back({"sdcard", true, 0});
			}
#endif
			return entries;
		}
		lv_fs_dir_close(&rootDir);
	}

	lv_fs_dir_t dir;
	Log::info(TAG, "Opening LVGL directory: %s", path.c_str());
	lv_fs_res_t res = lv_fs_dir_open(&dir, path.c_str());

	if (res != LV_FS_RES_OK) {
		Log::error(TAG, "Failed to open directory: %s (res=%d)", path.c_str(), res);
		flx::core::GuiLock::unlock();
		return entries;
	}
	Log::info(TAG, "Directory opened successfully");

	char fn[256];
	int count = 0;
	Log::info(TAG, "Starting directory read loop");
	while (true) {
		lv_fs_res_t readRes = lv_fs_dir_read(&dir, fn, sizeof(fn));
		if (readRes != LV_FS_RES_OK) {
			Log::info(TAG, "End of directory or error (res=%d)", readRes);
			break;
		}

		Log::info(TAG, "Read entry %d: %s", count++, fn);
		if (fn[0] == '\0') {
			break;
		}

		bool const is_dir = (fn[0] == '/');
		const char* name = is_dir ? fn + 1 : fn;

		// Skip . and ..
		if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
			continue;
		}

		FileEntry entry;
		entry.name = name;
		entry.isDirectory = is_dir;
		entry.size = 0;

		// Get file size if it's a file
		if (!is_dir) {
			std::string fullPath = buildPath(toVfsPath(path), name);
			struct stat st {};
			if (stat(fullPath.c_str(), &st) == 0) {
				entry.size = st.st_size;
			}
		}

		entries.push_back(entry);
	}

	lv_fs_dir_close(&dir);
	flx::core::GuiLock::unlock();
	return entries;
}

int FileSystemService::copyFile(const char* src, const char* dst, ProgressCallback callback) {
	struct stat st {};
	long totalSize = 0;
	if (stat(src, &st) == 0) {
		totalSize = st.st_size;
	}

	FILE* fsrc = fopen(src, "rb");
	if (!fsrc) {
		Log::error(TAG, "Failed to open source file for copying: %s", src);
		return -1;
	}

	FILE* fdst = fopen(dst, "wb");
	if (!fdst) {
		Log::error(TAG, "Failed to open destination file for copying: %s", dst);
		fclose(fsrc);
		return -1;
	}

	Log::info(TAG, "Copying file: %s -> %s (%ld bytes)", src, dst, totalSize);

	char buf[4096];
	size_t n = 0;
	long copied = 0;

	while ((n = fread(buf, 1, sizeof(buf), fsrc)) > 0) {
		if (fwrite(buf, 1, n, fdst) != n) {
			fclose(fsrc);
			fclose(fdst);
			return -1;
		}
		copied += n;

		if (callback && totalSize > 0) {
			int percent = (int)(copied * 100 / totalSize);
			callback(percent, src);
		}
	}

	fclose(fsrc);
	fclose(fdst);
	Log::info(TAG, "Copy completed: %s", dst);
	return 0;
}

int FileSystemService::copyRecursive(const char* src, const char* dst, ProgressCallback callback) {
	struct stat st {};
	if (stat(src, &st) != 0) {
		return -1;
	}

	if (S_ISDIR(st.st_mode)) {
		if (callback) {
			callback(0, src);
		}

		if (::mkdir(dst, 0777) != 0 && errno != EEXIST) {
			return -1;
		}

		DIR* d = opendir(src);
		if (!d) {
			return -1;
		}

		struct dirent* p = nullptr;
		int res = 0;

		while ((p = readdir(d))) {
			if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..")) {
				continue;
			}

			std::string subSrc = buildPath(src, p->d_name);
			std::string subDst = buildPath(dst, p->d_name);

			if (copyRecursive(subSrc.c_str(), subDst.c_str(), callback) != 0) {
				res = -1;
				break;
			}
		}

		closedir(d);
		return res;
	} else {
		return copyFile(src, dst, callback);
	}
}

bool FileSystemService::copy(const std::string& src, const std::string& dst, ProgressCallback callback) {
	return copyRecursive(src.c_str(), dst.c_str(), callback) == 0;
}

bool FileSystemService::move(const std::string& src, const std::string& dst) {

	if (rename(src.c_str(), dst.c_str()) == 0) {
		Log::info(TAG, "Moved: %s -> %s", src.c_str(), dst.c_str());
		return true;
	}

	Log::error(TAG, "Move failed: %s -> %s (errno: %d)", src.c_str(), dst.c_str(), errno);
	return false;
}

int FileSystemService::removeRecursive(const char* path, ProgressCallback callback) {
	DIR* d = opendir(path);
	if (!d) {
		// Not a directory, try to unlink as a file
		return unlink(path);
	}

	struct dirent* p = nullptr;
	int r = 0;

	while ((p = readdir(d))) {
		if (callback) {
			callback(0, path);
		}

		if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..")) {
			continue;
		}

		std::string subPath = buildPath(path, p->d_name);
		struct stat st {};

		if (stat(subPath.c_str(), &st) == 0) {
			if (S_ISDIR(st.st_mode)) {
				r = removeRecursive(subPath.c_str(), callback);
			} else {
				r = unlink(subPath.c_str());
			}
		} else {
			r = -1;
		}

		if (r != 0) {
			break;
		}
	}

	closedir(d);

	if (r == 0) {
		r = rmdir(path);
	}

	return r;
}

bool FileSystemService::remove(const std::string& path, ProgressCallback callback) {

	struct stat st {};
	if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
		Log::info(TAG, "Removing directory: %s", path.c_str());
		return removeRecursive(path.c_str(), callback) == 0;
	} else {
		Log::info(TAG, "Removing file: %s", path.c_str());
		return unlink(path.c_str()) == 0;
	}
}

bool FileSystemService::mkdir(const std::string& path) {

	if (::mkdir(path.c_str(), 0777) == 0) {
		Log::info(TAG, "Created directory: %s", path.c_str());
		return true;
	}

	if (errno == EEXIST) {
		return true;
	}

	Log::error(TAG, "Failed to create directory: %s (errno: %d)", path.c_str(), errno);
	return false;
}

} // namespace flx::services
