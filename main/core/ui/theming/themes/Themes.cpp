#include "Themes.hpp"
#include "core/common/Logger.hpp"
#include "core/lv_obj.h"
#include "core/ui/keyboard/VirtualKeyboard.hpp"
#include "misc/lv_color.h"
#include "misc/lv_palette.h"
#include "misc/lv_types.h"
#include "widgets/textarea/lv_textarea.h"
#include <string_view>

static constexpr std::string_view TAG = "Themes";

namespace Themes {
ThemeConfig GetConfig(ThemeType type) {
	ThemeConfig cfg {};
	cfg.apply_cb = nullptr;
	switch (type) {
		case ThemeType::HYPRLAND:
			cfg.primary = lv_palette_main(LV_PALETTE_CYAN);
			cfg.secondary = lv_palette_main(LV_PALETTE_PINK);
			cfg.surface = lv_palette_darken(LV_PALETTE_GREY, 4);
			cfg.on_primary = lv_color_white();
			cfg.text_primary = lv_color_white();
			cfg.text_secondary = lv_palette_lighten(LV_PALETTE_GREY, 2);
			cfg.error = lv_palette_main(LV_PALETTE_RED);
			cfg.dark = true;
			break;
		case ThemeType::MATERIAL:
			cfg.primary = lv_palette_main(LV_PALETTE_INDIGO);
			cfg.secondary = lv_palette_main(LV_PALETTE_AMBER);
			cfg.surface = lv_color_white();
			cfg.on_primary = lv_color_white();
			cfg.text_primary = lv_palette_darken(LV_PALETTE_GREY, 4);
			cfg.text_secondary = lv_palette_main(LV_PALETTE_GREY);
			cfg.error = lv_palette_main(LV_PALETTE_RED);
			cfg.dark = false;
			break;
		default:
			cfg.primary = lv_palette_main(LV_PALETTE_CYAN);
			cfg.secondary = lv_palette_main(LV_PALETTE_PINK);
			cfg.surface = lv_palette_darken(LV_PALETTE_GREY, 4);
			cfg.on_primary = lv_color_white();
			cfg.text_primary = lv_color_white();
			cfg.text_secondary = lv_palette_lighten(LV_PALETTE_GREY, 2);
			cfg.error = lv_palette_main(LV_PALETTE_RED);
			cfg.dark = true;
			break;
	}
	return cfg;
}

const char* ToString(ThemeType type) {
	switch (type) {
		case ThemeType::HYPRLAND:
			return "Hyprland";
		case ThemeType::MATERIAL:
			return "Material";
		default:
			return "Unknown";
	}
}

void ApplyGlobal(lv_theme_t* th, lv_obj_t* obj) {
	LV_UNUSED(th);

	if (lv_obj_check_type(obj, &lv_textarea_class)) {
		Log::debug(TAG, "Registering textarea for virtual keyboard");
		VirtualKeyboard::getInstance().register_input_area(obj);
	}
}

} // namespace Themes
