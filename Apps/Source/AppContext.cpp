#include <flx/apps/AppContext.hpp>
#include <flx/apps/AppManifest.hpp>

namespace flx::apps {

const std::string& AppContext::getAppId() const {
	static const std::string empty;
	return m_manifest ? m_manifest->appId : empty;
}

AppPaths AppContext::getPaths() const {
	return AppPaths(getAppId());
}

flx::core::Preferences AppContext::getPreferences() const {
	return flx::core::Preferences("app." + getAppId());
}

} // namespace flx::apps
