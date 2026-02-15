#pragma once

#include "lvgl.h"
#include <cstdint>
#include <flx/core/Observable.hpp>
#include <flx/core/Singleton.hpp>

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
	lv_subject_t m_theme_subject;
	lv_subject_t m_glass_enabled_subject;
	lv_subject_t m_transparency_enabled_subject;
	lv_subject_t m_wallpaper_enabled_subject;

	// Helper to sync Flx Observable -> LVGL Subject
	void syncFromSystem(int32_t value, lv_subject_t* subject);
};

} // namespace flx::ui::theming
