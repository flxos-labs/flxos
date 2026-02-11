#include "AppContext.hpp"
#include "AppManifest.hpp"

namespace System::Apps {

const std::string& AppContext::getAppId() const {
	static const std::string empty;
	return m_manifest ? m_manifest->appId : empty;
}

} // namespace System::Apps
