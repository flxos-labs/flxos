#include "core/lv_obj.h"
#include "misc/lv_color.h"
#include "misc/lv_palette.h"
#include "misc/lv_types.h"
#include "widgets/textarea/lv_textarea.h"
#include <flx/core/Logger.hpp>
#include <flx/ui/keyboard/VirtualKeyboard.hpp>
#include <flx/ui/theming/themes/Themes.hpp>
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
			cfg.success = lv_palette_main(LV_PALETTE_GREEN);
			cfg.warning = lv_palette_main(LV_PALETTE_AMBER);
			cfg.info = lv_palette_main(LV_PALETTE_BLUE);
			cfg.muted = lv_palette_main(LV_PALETTE_GREY);
			cfg.separator = lv_palette_darken(LV_PALETTE_GREY, 2);
			cfg.overlay_bg = lv_palette_darken(LV_PALETTE_BLUE_GREY, 4);
			cfg.overlay_text = lv_palette_lighten(LV_PALETTE_BLUE_GREY, 4);
			cfg.card_border = lv_palette_lighten(LV_PALETTE_GREY, 1);
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
			cfg.success = lv_palette_main(LV_PALETTE_GREEN);
			cfg.warning = lv_palette_main(LV_PALETTE_AMBER);
			cfg.info = lv_palette_main(LV_PALETTE_BLUE);
			cfg.muted = lv_palette_darken(LV_PALETTE_GREY, 1);
			cfg.separator = lv_palette_lighten(LV_PALETTE_GREY, 2);
			cfg.overlay_bg = lv_palette_darken(LV_PALETTE_BLUE_GREY, 4);
			cfg.overlay_text = lv_color_white();
			cfg.card_border = lv_palette_lighten(LV_PALETTE_GREY, 3);
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
			cfg.success = lv_palette_main(LV_PALETTE_GREEN);
			cfg.warning = lv_palette_main(LV_PALETTE_AMBER);
			cfg.info = lv_palette_main(LV_PALETTE_BLUE);
			cfg.muted = lv_palette_main(LV_PALETTE_GREY);
			cfg.separator = lv_palette_darken(LV_PALETTE_GREY, 2);
			cfg.overlay_bg = lv_palette_darken(LV_PALETTE_BLUE_GREY, 4);
			cfg.overlay_text = lv_palette_lighten(LV_PALETTE_BLUE_GREY, 4);
			cfg.card_border = lv_palette_lighten(LV_PALETTE_GREY, 1);
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
