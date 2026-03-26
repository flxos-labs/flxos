#include <cstdint>
#include <flx/core/Logger.hpp>
#include <flx/system/managers/SettingsManager.hpp>
#include <flx/system/managers/ThemeManager.hpp>

static constexpr const char* TAG = "ThemeManager";

namespace flx::system {

const flx::services::ServiceManifest ThemeManager::serviceManifest = {
	.serviceId = "com.flxos.theme",
	.serviceName = "Theme",
	.dependencies = {"com.flxos.settings"},
	.priority = 25,
	.required = false,
	.autoStart = true,
	.guiRequired = false,
	.capabilities = flx::services::ServiceCapability::None,
	.description = "Theme engine and glass effects",
};

bool ThemeManager::onStart() {
	SettingsManager::getInstance().registerSetting("theme", m_theme_subject);
	SettingsManager::getInstance().registerSetting("glass_enabled", m_glass_enabled_subject);
	SettingsManager::getInstance().registerSetting("transp_enabled", m_transparency_enabled_subject);

	Log::info(TAG, "Theme service started");
	return true;
}

void ThemeManager::onStop() {
	Log::info(TAG, "Theme service stopped");
}

} // namespace flx::system
