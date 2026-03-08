#pragma once

#include <string>

namespace flx::apps {

class AppPaths {
public:

	explicit AppPaths(std::string appId);

	std::string getDataDir() const;
	std::string getDataPath(const std::string& child) const;

	std::string getCacheDir() const;

	std::string getAssetsDir() const;
	std::string getAssetsPath(const std::string& child) const;

	bool ensureDirectories() const;

private:

	std::string m_appId;
};

} // namespace flx::apps
