#include <flx/apps/AppPaths.hpp>

#include <cerrno>
#include <cstring>
#include <flx/core/Logger.hpp>
#include <sys/stat.h>
#include <sys/types.h>
#include <utility>

namespace flx::apps {
namespace {

bool ensureDirectoryExists(const std::string& path) {
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

	Log::error("AppPaths", "Failed to ensure directory %s: %s", path.c_str(), std::strerror(errno));
	return false;
}

std::string joinPath(const std::string& base, const std::string& child) {
	if (child.empty()) {
		return base;
	}
	if (!child.empty() && child.front() == '/') {
		return base + child;
	}
	return base + "/" + child;
}

} // namespace

AppPaths::AppPaths(std::string appId) : m_appId(std::move(appId)) {}

std::string AppPaths::getDataDir() const {
	return "/data/apps/" + m_appId;
}

std::string AppPaths::getDataPath(const std::string& child) const {
	return joinPath(getDataDir(), child);
}

std::string AppPaths::getCacheDir() const {
	return getDataDir() + "/.cache";
}

std::string AppPaths::getAssetsDir() const {
	return "/assets/apps/" + m_appId;
}

std::string AppPaths::getAssetsPath(const std::string& child) const {
	return joinPath(getAssetsDir(), child);
}

bool AppPaths::ensureDirectories() const {
	return ensureDirectoryExists("/data")
		&& ensureDirectoryExists("/data/apps")
		&& ensureDirectoryExists(getDataDir())
		&& ensureDirectoryExists(getCacheDir());
}

} // namespace flx::apps
