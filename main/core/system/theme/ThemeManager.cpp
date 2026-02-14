#include "ThemeManager.hpp"
#include "core/system/settings/SettingsManager.hpp"
#include "core/ui/theming/themes/Themes.hpp"
#include <cstdint>
#include <flx/core/Logger.hpp>

#if !CONFIG_FLXOS_HEADLESS_MODE
#include "core/tasks/gui/GuiTask.hpp"
#include "core/ui/LvglBridgeHelpers.hpp"
#include "core/ui/theming/theme_engine/ThemeEngine.hpp"
#endif

static constexpr const char* TAG = "ThemeManager";

namespace System {

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

#if !CONFIG_FLXOS_HEADLESS_MODE
void ThemeManager::onGuiInit() {
	GuiTask::lock();

	INIT_INT_BRIDGE(m_theme_bridge, m_theme_subject, applyTheme);
	INIT_BRIDGE(m_glass_enabled_bridge, m_glass_enabled_subject);
	INIT_BRIDGE(m_transparency_enabled_bridge, m_transparency_enabled_subject);
	INIT_BRIDGE(m_wallpaper_enabled_bridge, m_wallpaper_enabled_subject);
	INIT_STRING_BRIDGE(m_wallpaper_path_bridge, m_wallpaper_path_subject);

	applyTheme(m_theme_subject.get());

	GuiTask::unlock();
}
#endif

void ThemeManager::applyTheme(int32_t theme) {
#if !CONFIG_FLXOS_HEADLESS_MODE
	ThemeEngine::set_theme((ThemeType)theme);
#endif
}

} // namespace System
