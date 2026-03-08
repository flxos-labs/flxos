#pragma once

#include <string>
#include <cerrno>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <flx/core/Logger.hpp>

namespace flx::core {

inline bool ensureDirectoryExists(const std::string& path) {
	if (path.empty()) {
		return false;
	}

	struct stat st {};
	if (stat(path.c_str(), &st) == 0) {
		return S_ISDIR(st.st_mode);
	}

	if (errno == ENOENT) {
		if (mkdir(path.c_str(), 0755) == 0) {
			return true;
		}
		if (errno == EEXIST) {
			return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
		}
	}

	Log::error("PathUtils", "Failed to ensure directory %s: %s", path.c_str(), std::strerror(errno));
	return false;
}

inline std::string sanitizeSegment(const std::string& segment) {
	std::string safe = segment;
	for (char& c : safe) {
		if (!std::isalnum(c) && c != '.' && c != '_' && c != '-') {
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
