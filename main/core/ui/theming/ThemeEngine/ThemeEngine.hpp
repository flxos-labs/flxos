#pragma once
#include "../Themes/Themes.hpp"
#include "lvgl.h"

class ThemeEngine {
public:

	static void init();
	static void set_theme(ThemeType theme, lv_display_t* disp = nullptr);
	static ThemeType get_current_theme();
	static void cycle_theme();

private:

	static ThemeType current_theme;
	static void apply_theme(ThemeType theme, lv_display_t* disp);

	static void cleanup_previous_theme(lv_display_t* disp);
};
