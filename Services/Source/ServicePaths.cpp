#include <flx/services/ServicePaths.hpp>

#include <cerrno>
#include <cstring>
#include <flx/core/Logger.hpp>
#include <sys/stat.h>
#include <sys/types.h>
#include <utility>

namespace flx::services {
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

	Log::error("ServicePaths", "Failed to ensure directory %s: %s", path.c_str(), std::strerror(errno));
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

ServicePaths::ServicePaths(std::string serviceId) : m_serviceId(std::move(serviceId)) {}

std::string ServicePaths::getDataDir() const {
	return "/data/services/" + m_serviceId;
}

std::string ServicePaths::getDataPath(const std::string& child) const {
	return joinPath(getDataDir(), child);
}

std::string ServicePaths::getCacheDir() const {
	return getDataDir() + "/.cache";
}

bool ServicePaths::ensureDirectories() const {
	return ensureDirectoryExists("/data") && ensureDirectoryExists("/data/services") && ensureDirectoryExists(getDataDir()) && ensureDirectoryExists(getCacheDir());
}

} // namespace flx::services
