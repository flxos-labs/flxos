#include <flx/apps/AppPaths.hpp>

#include <cerrno>
#include <cstring>
#include <flx/core/Logger.hpp>
#include <flx/core/PathUtils.hpp>
#include <utility>

namespace flx::apps {

AppPaths::AppPaths(std::string appId) : m_appId(flx::core::sanitizeSegment(std::move(appId))) {
	if (m_appId.empty() || m_appId == "." || m_appId == "..") {
		flx::Log::error("AppPaths", "Invalid app ID: reserved segment");
		m_appId = "_invalid_";
	}
}

std::string AppPaths::getDataDir() const {
	return "/data/apps/" + m_appId;
}

std::string AppPaths::getDataPath(const std::string& child) const {
	return flx::core::joinPath(getDataDir(), child);
}

std::string AppPaths::getCacheDir() const {
	return getDataDir() + "/.cache";
}

std::string AppPaths::getAssetsDir() const {
	return "/assets/apps/" + m_appId;
}

std::string AppPaths::getAssetsPath(const std::string& child) const {
	return flx::core::joinPath(getAssetsDir(), child);
}

bool AppPaths::ensureDirectories() const {
	return flx::core::ensureDirectoryExists("/data") && flx::core::ensureDirectoryExists("/data/apps") && flx::core::ensureDirectoryExists(getDataDir()) && flx::core::ensureDirectoryExists(getCacheDir());
}

} // namespace flx::apps
