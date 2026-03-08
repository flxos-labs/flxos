#include <flx/services/ServicePaths.hpp>

#include <cerrno>
#include <cstring>
#include <utility>
#include <flx/core/PathUtils.hpp>

namespace flx::services {

ServicePaths::ServicePaths(std::string serviceId) : m_serviceId(flx::core::sanitizeSegment(std::move(serviceId))) {}

std::string ServicePaths::getDataDir() const {
	return "/data/services/" + m_serviceId;
}

std::string ServicePaths::getDataPath(const std::string& child) const {
	return flx::core::joinPath(getDataDir(), child);
}

std::string ServicePaths::getCacheDir() const {
	return getDataDir() + "/.cache";
}

bool ServicePaths::ensureDirectories() const {
	return flx::core::ensureDirectoryExists("/data") && flx::core::ensureDirectoryExists("/data/services") && flx::core::ensureDirectoryExists(getDataDir()) && flx::core::ensureDirectoryExists(getCacheDir());
}

} // namespace flx::services
