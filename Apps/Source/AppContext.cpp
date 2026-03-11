#include <flx/apps/AppContext.hpp>

namespace flx::apps {

const std::string& AppContext::getAppId() const {
	return m_manifest.appId;
}

AppPaths AppContext::getPaths() const {
	return AppPaths(getAppId());
}

flx::core::Preferences AppContext::getPreferences() const {
	return flx::core::Preferences("app." + getAppId());
}

} // namespace flx::apps
