#pragma once
#include "lvgl.h"

enum class ThemeType { HYPRLAND,
					   MATERIAL };

struct ThemeConfig {
	lv_color_t primary;
	lv_color_t secondary;
	bool dark;
	lv_theme_apply_cb_t apply_cb;
};

namespace Themes {
ThemeConfig GetConfig(ThemeType type);
const char* ToString(ThemeType type);
void ApplyGlobal(lv_theme_t* th, lv_obj_t* obj);
} // namespace Themes
