#pragma once

#include <cerrno>
#include <cstring>
#include <flx/core/Logger.hpp>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>

namespace flx::core {

inline bool ensureDirectoryExists(const std::string& path) {
	if (path.empty()) {
		return false;
	}

	struct stat st {};
	if (stat(path.c_str(), &st) == 0) {
		return S_ISDIR(st.st_mode);
	}

	// Recursive directory creation
	size_t pos = 0;
	do {
		pos = path.find('/', pos + 1);
		std::string subdir = (pos == std::string::npos) ? path : path.substr(0, pos);
		if (subdir.empty()) {
			continue;
		}
		struct stat sub_st {};
		if (stat(subdir.c_str(), &sub_st) != 0) {
			if (errno == ENOENT) {
				if (mkdir(subdir.c_str(), 0755) != 0 && errno != EEXIST) {
					Log::error("PathUtils", "Failed to create directory %s: %s", subdir.c_str(), std::strerror(errno));
					return false;
				}
			} else {
				Log::error("PathUtils", "Failed to stat subdirectory %s: %s", subdir.c_str(), std::strerror(errno));
				return false;
			}
		} else if (!S_ISDIR(sub_st.st_mode)) {
			Log::error("PathUtils", "Path segment %s exists but is not a directory", subdir.c_str());
			return false;
		}
	} while (pos != std::string::npos);

	return true;
}

inline std::string sanitizeSegment(const std::string& segment) {
	std::string safe = segment;
	for (char& c: safe) {
		if (!std::isalnum(static_cast<unsigned char>(c)) && c != '.' && c != '_' && c != '-') {
			c = '_';
		}
	}
	return safe;
}

inline std::string joinPath(const std::string& base, const std::string& child) {
	if (child.empty()) {
		return base;
	}
	if (child.find("..") != std::string::npos) {
		Log::error("PathUtils", "Path traversal detected: %s", child.c_str());
		return base;
	}
	if (!child.empty() && child.front() == '/') {
		return base + child;
	}
	return base + "/" + child;
}

} // namespace flx::core
