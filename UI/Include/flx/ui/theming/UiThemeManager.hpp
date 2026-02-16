#pragma once

#include "lvgl.h"
#include <cstdint>
#include <flx/core/Observable.hpp>
#include <flx/core/Singleton.hpp>
#include <flx/ui/LvglObserverBridge.hpp>
#include <memory>

namespace flx::ui::theming {

class UiThemeManager : public flx::Singleton<UiThemeManager> {
	friend class flx::Singleton<UiThemeManager>;

public:

	void init();

	// Expose LVGL subjects for UI components to bind to
	lv_subject_t* getThemeSubject();
	lv_subject_t* getGlassEnabledSubject();
	lv_subject_t* getTransparencyEnabledSubject();
	lv_subject_t* getWallpaperEnabledSubject();

private:

	UiThemeManager() = default;
	~UiThemeManager();

	// LVGL subjects (integers)
	// LVGL Bridges
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_theme_bridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_glass_enabled_bridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_transparency_enabled_bridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_wallpaper_enabled_bridge;
};

} // namespace flx::ui::theming
