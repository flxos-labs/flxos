#pragma once

#include <flx/core/Observable.hpp>
#include <flx/core/Singleton.hpp>
#include <flx/services/IService.hpp>
#include <flx/services/ServiceManifest.hpp>
#include <memory>
#include <string>

namespace flx::system {

class ThemeManager : public flx::Singleton<ThemeManager>, public flx::services::IService {
	friend class flx::Singleton<ThemeManager>;

public:

	// ──── IService manifest ────
	static const flx::services::ServiceManifest serviceManifest;
	const flx::services::ServiceManifest& getManifest() const override { return serviceManifest; }

	// ──── IService lifecycle ────
	bool onStart() override;
	void onStop() override;

	flx::Observable<int32_t>& getThemeObservable() { return m_theme_subject; }
	flx::Observable<int32_t>& getGlassEnabledObservable() { return m_glass_enabled_subject; }
	flx::Observable<int32_t>& getTransparencyEnabledObservable() { return m_transparency_enabled_subject; }
	flx::Observable<int32_t>& getWallpaperEnabledObservable() { return m_wallpaper_enabled_subject; }
	flx::StringObservable& getWallpaperPathObservable() { return m_wallpaper_path_subject; }

private:

	ThemeManager() = default;
	~ThemeManager() = default;

	flx::Observable<int32_t> m_theme_subject {1};
	flx::Observable<int32_t> m_glass_enabled_subject {0};
	flx::Observable<int32_t> m_transparency_enabled_subject {0};
	flx::Observable<int32_t> m_wallpaper_enabled_subject {0};
	flx::StringObservable m_wallpaper_path_subject {""};

};

} // namespace flx::system
