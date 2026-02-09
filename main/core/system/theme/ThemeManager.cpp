#include "ThemeManager.hpp"
#include "core/system/settings/SettingsManager.hpp"
#include "core/ui/theming/themes/Themes.hpp"
#include <cstdint>

#if !CONFIG_FLXOS_HEADLESS_MODE
#include "core/tasks/gui/GuiTask.hpp"
#include "core/ui/LvglBridgeHelpers.hpp"
#include "core/ui/theming/theme_engine/ThemeEngine.hpp"
#endif

namespace System {

void ThemeManager::init() {
	SettingsManager::getInstance().registerSetting("theme", m_theme_subject);
	SettingsManager::getInstance().registerSetting("glass_enabled", m_glass_enabled_subject);
	SettingsManager::getInstance().registerSetting("transp_enabled", m_transparency_enabled_subject);
	SettingsManager::getInstance().registerSetting("wp_enabled", m_wallpaper_enabled_subject);
	SettingsManager::getInstance().registerSetting("wp_path", m_wallpaper_path_subject);

	// Set default wallpaper path if not already set
	if (strlen(m_wallpaper_path_subject.get()) == 0) {
		m_wallpaper_path_subject.set(DEFAULT_WALLPAPER_PATH);
	}

	// Initial application
	applyTheme(m_theme_subject.get());
}

#if !CONFIG_FLXOS_HEADLESS_MODE
void ThemeManager::initGuiBridges() {
	GuiTask::lock();

	INIT_INT_BRIDGE(m_theme_bridge, m_theme_subject, applyTheme);
	INIT_BRIDGE(m_glass_enabled_bridge, m_glass_enabled_subject);
	INIT_BRIDGE(m_transparency_enabled_bridge, m_transparency_enabled_subject);
	INIT_BRIDGE(m_wallpaper_enabled_bridge, m_wallpaper_enabled_subject);
	INIT_STRING_BRIDGE(m_wallpaper_path_bridge, m_wallpaper_path_subject);

	// Initial application to GUI
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
