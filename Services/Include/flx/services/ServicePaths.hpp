#pragma once

#include <string>

namespace flx::services {

class ServicePaths {
public:

	explicit ServicePaths(std::string serviceId);

	std::string getDataDir() const;
	std::string getDataPath(const std::string& child) const;
	std::string getCacheDir() const;

	bool ensureDirectories() const;

private:

	std::string m_serviceId;
};

} // namespace flx::services
