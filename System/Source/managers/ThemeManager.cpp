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
	.description = "Theme engine, wallpaper, and glass effects",
};

bool ThemeManager::onStart() {
	SettingsManager::getInstance().registerSetting("theme", m_theme_subject);
	SettingsManager::getInstance().registerSetting("glass_enabled", m_glass_enabled_subject);
	SettingsManager::getInstance().registerSetting("transp_enabled", m_transparency_enabled_subject);
	SettingsManager::getInstance().registerSetting("wp_enabled", m_wallpaper_enabled_subject);
	SettingsManager::getInstance().registerSetting("wp_path", m_wallpaper_path_subject);

	applyTheme(m_theme_subject.get());
	Log::info(TAG, "Theme service started");
	return true;
}

void ThemeManager::onStop() {
	Log::info(TAG, "Theme service stopped");
}

void ThemeManager::applyTheme(int32_t theme) {
	// Theme application handled by UI component based on observable state
}

} // namespace flx::system
