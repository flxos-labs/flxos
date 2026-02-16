#include "SettingsApp.hpp"
#include <flx/apps/AppManifest.hpp>

using namespace flx::apps;

namespace System::Apps {

const AppManifest SettingsApp::manifest = {
	.appId = "com.flxos.settings",
	.appName = "Settings",
	.appIcon = LV_SYMBOL_SETTINGS,
	.appVersionName = "1.1.0",
	.appVersionCode = 2,
	.category = AppCategory::System,
	.flags = AppFlags::SingleInstance,
	.location = AppLocation::internal(),
	.description = "System configuration and preferences",
	.sortPriority = 10,
	.capabilities = AppCapability::WiFi | AppCapability::Bluetooth,
	.createApp = []() -> std::shared_ptr<App> { return std::make_shared<SettingsApp>(); }
};

} // namespace System::Apps
