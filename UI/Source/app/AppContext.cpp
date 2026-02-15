#include <flx/ui/app/AppContext.hpp>
#include <flx/ui/app/AppManifest.hpp>

namespace flx::app {

const std::string& AppContext::getAppId() const {
	static const std::string empty;
	return m_manifest ? m_manifest->appId : empty;
}

} // namespace flx::app
