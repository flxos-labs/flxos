#include "ThemeManager.hpp"
#include "core/system/settings/SettingsManager.hpp"

#if !CONFIG_FLXOS_HEADLESS_MODE
#include "core/tasks/gui/GuiTask.hpp"
#include "core/ui/theming/theme_engine/ThemeEngine.hpp"
#endif

namespace System {

ThemeManager& ThemeManager::getInstance() {
	static ThemeManager instance;
	return instance;
}

void ThemeManager::init() {
	SettingsManager::getInstance().registerSetting("theme", m_theme_subject);
	SettingsManager::getInstance().registerSetting("glass_enabled", m_glass_enabled_subject);
	SettingsManager::getInstance().registerSetting("transp_enabled", m_transparency_enabled_subject);
	SettingsManager::getInstance().registerSetting("wp_enabled", m_wallpaper_enabled_subject);
	SettingsManager::getInstance().registerSetting("wp_path", m_wallpaper_path_subject);

	// Initial application
	applyTheme(m_theme_subject.get());
}

#if !CONFIG_FLXOS_HEADLESS_MODE
void ThemeManager::initGuiBridges() {
	GuiTask::lock();
	m_theme_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_theme_subject);
	m_glass_enabled_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_glass_enabled_subject);
	m_transparency_enabled_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_transparency_enabled_subject);
	m_wallpaper_enabled_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_wallpaper_enabled_subject);
	m_wallpaper_path_bridge = std::make_unique<LvglStringObserverBridge>(m_wallpaper_path_subject);

	lv_subject_add_observer(m_theme_bridge->getSubject(), [](lv_observer_t*, lv_subject_t* s) { ThemeManager::getInstance().applyTheme(lv_subject_get_int(s)); }, nullptr);

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
