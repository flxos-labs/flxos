#include "Themes.hpp"
#include "core/common/Logger.hpp"
#include "core/ui/VirtualKeyboard/VirtualKeyboard.hpp"
#include <string_view>

static constexpr std::string_view TAG = "Themes";

namespace Themes {
ThemeConfig GetConfig(ThemeType type) {
	ThemeConfig cfg;
	cfg.apply_cb = nullptr;
	switch (type) {
		case ThemeType::HYPRLAND:
			cfg.primary = lv_palette_main(LV_PALETTE_CYAN);
			cfg.secondary = lv_palette_main(LV_PALETTE_PINK);
			cfg.dark = true;
			break;
		case ThemeType::MATERIAL:
			cfg.primary = lv_palette_main(LV_PALETTE_INDIGO);
			cfg.secondary = lv_palette_main(LV_PALETTE_AMBER);
			cfg.dark = false;
			break;
		default:
			cfg.primary = lv_palette_main(LV_PALETTE_CYAN);
			cfg.secondary = lv_palette_main(LV_PALETTE_PINK);
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
