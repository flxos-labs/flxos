#pragma once

#include <flx/core/Observable.hpp>
#include <flx/core/Singleton.hpp>
#include "core/services/IService.hpp"
#include "core/services/ServiceManifest.hpp"
#include <memory>
#include <string>

#if !CONFIG_FLXOS_HEADLESS_MODE
#include "core/ui/LvglObserverBridge.hpp"
#include "lvgl.h"
#endif

namespace System {

class ThemeManager : public flx::Singleton<ThemeManager>, public Services::IService {
	friend class flx::Singleton<ThemeManager>;

public:

	// ──── IService manifest ────
	static const Services::ServiceManifest serviceManifest;
	const Services::ServiceManifest& getManifest() const override { return serviceManifest; }

	// ──── IService lifecycle ────
	bool onStart() override;
	void onStop() override;

#if !CONFIG_FLXOS_HEADLESS_MODE
	void onGuiInit() override;
#endif

	flx::Observable<int32_t>& getThemeObservable() { return m_theme_subject; }
	flx::Observable<int32_t>& getGlassEnabledObservable() { return m_glass_enabled_subject; }
	flx::Observable<int32_t>& getTransparencyEnabledObservable() { return m_transparency_enabled_subject; }
	flx::Observable<int32_t>& getWallpaperEnabledObservable() { return m_wallpaper_enabled_subject; }
	flx::StringObservable& getWallpaperPathObservable() { return m_wallpaper_path_subject; }

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

	flx::Observable<int32_t> m_theme_subject {1};
	flx::Observable<int32_t> m_glass_enabled_subject {0};
	flx::Observable<int32_t> m_transparency_enabled_subject {0};
	flx::Observable<int32_t> m_wallpaper_enabled_subject {0};
	flx::StringObservable m_wallpaper_path_subject {""};

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
