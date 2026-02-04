#pragma once

#include "core/common/Observable.hpp"
#include <memory>
#include <string>

#if !CONFIG_FLXOS_HEADLESS_MODE
#include "core/ui/LvglObserverBridge.hpp"
#include "lvgl.h"
#endif

namespace System {

class ThemeManager {
public:

	static ThemeManager& getInstance();

	void init();

#if !CONFIG_FLXOS_HEADLESS_MODE
	void initGuiBridges();
#endif

	Observable<int32_t>& getThemeObservable() { return m_theme_subject; }
	Observable<int32_t>& getGlassEnabledObservable() { return m_glass_enabled_subject; }
	Observable<int32_t>& getTransparencyEnabledObservable() { return m_transparency_enabled_subject; }
	Observable<int32_t>& getWallpaperEnabledObservable() { return m_wallpaper_enabled_subject; }
	StringObservable& getWallpaperPathObservable() { return m_wallpaper_path_subject; }

#if !CONFIG_FLXOS_HEADLESS_MODE
	lv_subject_t& getThemeSubject() { return *m_theme_bridge->getSubject(); }
	lv_subject_t& getGlassEnabledSubject() { return *m_glass_enabled_bridge->getSubject(); }
	lv_subject_t& getTransparencyEnabledSubject() { return *m_transparency_enabled_bridge->getSubject(); }
	lv_subject_t& getWallpaperEnabledSubject() { return *m_wallpaper_enabled_bridge->getSubject(); }
	lv_subject_t& getWallpaperPathSubject() { return *m_wallpaper_path_bridge->getSubject(); }
#endif

private:

	ThemeManager() = default;
	~ThemeManager() = default;
	ThemeManager(const ThemeManager&) = delete;
	ThemeManager& operator=(const ThemeManager&) = delete;

	Observable<int32_t> m_theme_subject {0};
	Observable<int32_t> m_glass_enabled_subject {0};
	Observable<int32_t> m_transparency_enabled_subject {1};
	Observable<int32_t> m_wallpaper_enabled_subject {0};
	StringObservable m_wallpaper_path_subject {""};

#if !CONFIG_FLXOS_HEADLESS_MODE
	std::unique_ptr<LvglObserverBridge<int32_t>> m_theme_bridge {};
	std::unique_ptr<LvglObserverBridge<int32_t>> m_glass_enabled_bridge {};
	std::unique_ptr<LvglObserverBridge<int32_t>> m_transparency_enabled_bridge {};
	std::unique_ptr<LvglObserverBridge<int32_t>> m_wallpaper_enabled_bridge {};
	std::unique_ptr<LvglStringObserverBridge> m_wallpaper_path_bridge {};
#endif

	static void applyTheme(int32_t theme);
};

} // namespace System
