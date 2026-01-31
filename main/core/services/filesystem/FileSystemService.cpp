#include "FileSystemService.hpp"
#include "esp_log.h"
#include "lvgl.h"
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const char* TAG = "FileSystemService";

namespace System {
namespace Services {

FileSystemService& FileSystemService::getInstance() {
	static FileSystemService instance;
	return instance;
}

std::string FileSystemService::toVfsPath(const std::string& lvPath) {
	if (lvPath.substr(0, 2) == "A:")
		return lvPath.substr(2);
	return lvPath;
}

std::string FileSystemService::buildPath(const std::string& base, const std::string& name) {
	return (base.back() == '/') ? base + name : base + "/" + name;
}

std::vector<FileEntry> FileSystemService::listDirectory(const std::string& path) {
	std::vector<FileEntry> entries;

	// Special handling for root "A:/" which may not exist as a real directory
	if (path == "A:/" || path == "A:") {
		// Check if actual directories exist, otherwise return defaults
		lv_fs_dir_t dir;
		lv_fs_res_t res = lv_fs_dir_open(&dir, path.c_str());

		if (res != LV_FS_RES_OK) {
			// Return default directories
			entries.push_back({"system", true, 0});
			entries.push_back({"data", true, 0});
			return entries;
		}
		lv_fs_dir_close(&dir);
	}

	lv_fs_dir_t dir;
	lv_fs_res_t res = lv_fs_dir_open(&dir, path.c_str());

	if (res != LV_FS_RES_OK) {
		ESP_LOGE(TAG, "Failed to open directory: %s", path.c_str());
		return entries;
	}

	char fn[256];
	while (lv_fs_dir_read(&dir, fn, sizeof(fn)) == LV_FS_RES_OK) {
		if (fn[0] == '\0')
			break;

		bool is_dir = (fn[0] == '/');
		const char* name = is_dir ? fn + 1 : fn;

		// Skip . and ..
		if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
			continue;

		FileEntry entry;
		entry.name = name;
		entry.isDirectory = is_dir;
		entry.size = 0;

		// Get file size if it's a file
		if (!is_dir) {
			std::string fullPath = buildPath(toVfsPath(path), name);
			struct stat st;
			if (stat(fullPath.c_str(), &st) == 0) {
				entry.size = st.st_size;
			}
		}

		entries.push_back(entry);
	}

	lv_fs_dir_close(&dir);
	ESP_LOGD(TAG, "Listed %d entries in %s", entries.size(), path.c_str());
	return entries;
}

int FileSystemService::copyFile(const char* src, const char* dst, ProgressCallback callback) {
	struct stat st;
	long totalSize = 0;
	if (stat(src, &st) == 0)
		totalSize = st.st_size;

	FILE* fsrc = fopen(src, "rb");
	if (!fsrc) {
		ESP_LOGE(TAG, "Failed to open source file: %s", src);
		return -1;
	}

	FILE* fdst = fopen(dst, "wb");
	if (!fdst) {
		ESP_LOGE(TAG, "Failed to open destination file: %s", dst);
		fclose(fsrc);
		return -1;
	}

	char buf[4096];
	size_t n;
	long copied = 0;

	while ((n = fread(buf, 1, sizeof(buf), fsrc)) > 0) {
		if (fwrite(buf, 1, n, fdst) != n) {
			ESP_LOGE(TAG, "Write error during copy");
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
	ESP_LOGD(TAG, "Copied file: %s -> %s (%ld bytes)", src, dst, copied);
	return 0;
}

int FileSystemService::copyRecursive(const char* src, const char* dst, ProgressCallback callback) {
	struct stat st;
	if (stat(src, &st) != 0) {
		ESP_LOGE(TAG, "Failed to stat: %s", src);
		return -1;
	}

	if (S_ISDIR(st.st_mode)) {
		if (callback)
			callback(0, src);

		if (::mkdir(dst, 0777) != 0 && errno != EEXIST) {
			ESP_LOGE(TAG, "Failed to create directory: %s", dst);
			return -1;
		}

		DIR* d = opendir(src);
		if (!d) {
			ESP_LOGE(TAG, "Failed to open directory: %s", src);
			return -1;
		}

		struct dirent* p;
		int res = 0;

		while ((p = readdir(d))) {
			if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
				continue;

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
	ESP_LOGI(TAG, "Copying: %s -> %s", src.c_str(), dst.c_str());
	return copyRecursive(src.c_str(), dst.c_str(), callback) == 0;
}

bool FileSystemService::move(const std::string& src, const std::string& dst) {
	ESP_LOGI(TAG, "Moving: %s -> %s", src.c_str(), dst.c_str());

	if (rename(src.c_str(), dst.c_str()) == 0) {
		return true;
	}

	ESP_LOGE(TAG, "Failed to move/rename: %s -> %s", src.c_str(), dst.c_str());
	return false;
}

int FileSystemService::removeRecursive(const char* path, ProgressCallback callback) {
	DIR* d = opendir(path);
	if (!d) {
		// Not a directory, try to unlink as a file
		return unlink(path);
	}

	struct dirent* p;
	int r = 0;

	while ((p = readdir(d))) {
		if (callback)
			callback(0, path);

		if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
			continue;

		std::string subPath = buildPath(path, p->d_name);
		struct stat st;

		if (stat(subPath.c_str(), &st) == 0) {
			if (S_ISDIR(st.st_mode))
				r = removeRecursive(subPath.c_str(), callback);
			else
				r = unlink(subPath.c_str());
		} else {
			r = -1;
		}

		if (r != 0)
			break;
	}

	closedir(d);

	if (r == 0)
		r = rmdir(path);

	return r;
}

bool FileSystemService::remove(const std::string& path, ProgressCallback callback) {
	ESP_LOGI(TAG, "Removing: %s", path.c_str());

	struct stat st;
	if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
		return removeRecursive(path.c_str(), callback) == 0;
	} else {
		return unlink(path.c_str()) == 0;
	}
}

bool FileSystemService::mkdir(const std::string& path) {
	ESP_LOGI(TAG, "Creating directory: %s", path.c_str());

	if (::mkdir(path.c_str(), 0777) == 0) {
		return true;
	}

	if (errno == EEXIST) {
		ESP_LOGW(TAG, "Directory already exists: %s", path.c_str());
		return true;
	}

	ESP_LOGE(TAG, "Failed to create directory: %s", path.c_str());
	return false;
}

} // namespace Services
} // namespace System
