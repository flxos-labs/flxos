#include <flx/system/managers/ThemeManager.hpp>
#include <flx/ui/GuiTask.hpp>
#include <flx/ui/theming/UiThemeManager.hpp>
#include <flx/ui/theming/theme_engine/ThemeEngine.hpp>

namespace flx::ui::theming {

UiThemeManager::~UiThemeManager() {
	// Smart pointers handle cleanup
}

void UiThemeManager::init() {
	auto& sys = flx::system::ThemeManager::getInstance();

	// Initialize Bridges (Bidirectional Sync)
	m_theme_bridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(sys.getThemeObservable());
	m_glass_enabled_bridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(sys.getGlassEnabledObservable());
	m_transparency_enabled_bridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(sys.getTransparencyEnabledObservable());
	m_wallpaper_enabled_bridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(sys.getWallpaperEnabledObservable());
	m_wallpaper_path_bridge = std::make_unique<flx::ui::LvglStringObserverBridge>(sys.getWallpaperPathObservable());

	// Additional subscription to trigger ThemeEngine update on theme change
	sys.getThemeObservable().subscribe([](const int32_t& val) {
		ThemeEngine::set_theme((ThemeType)val);
	});
	
	// Apply the initial theme state loaded from settings
	ThemeEngine::set_theme((ThemeType)sys.getThemeObservable().get());
}

lv_subject_t* UiThemeManager::getThemeSubject() { return m_theme_bridge->getSubject(); }
lv_subject_t* UiThemeManager::getGlassEnabledSubject() { return m_glass_enabled_bridge->getSubject(); }
lv_subject_t* UiThemeManager::getTransparencyEnabledSubject() { return m_transparency_enabled_bridge->getSubject(); }
lv_subject_t* UiThemeManager::getWallpaperEnabledSubject() { return m_wallpaper_enabled_bridge->getSubject(); }
lv_subject_t* UiThemeManager::getWallpaperPathSubject() { return m_wallpaper_path_bridge->getSubject(); }

} // namespace flx::ui::theming
