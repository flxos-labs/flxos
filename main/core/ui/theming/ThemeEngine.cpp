#include "ThemeEngine.hpp"
#include "core/system/SystemManager.hpp"
#include "core/tasks/gui/GuiTask.hpp"
#include "esp_log.h"
#include <map>
#include <vector>

ThemeType ThemeEngine::current_theme = ThemeType::MATERIAL;
static std::map<lv_display_t*, std::vector<lv_theme_t*>> engine_themes;

void ThemeEngine::init() {
	lv_display_t* disp = lv_display_get_default();
	if (disp) {
		apply_theme(current_theme, disp);
		ESP_LOGI("ThemeEngine", "Initialized");
	} else {
		ESP_LOGE("ThemeEngine", "Failed to initialize: No default display found");
	}
}

void ThemeEngine::set_theme(ThemeType theme, lv_display_t* disp) {
	GuiTask::lock();
	if (!disp)
		disp = lv_display_get_default();
	if (!disp) {
		GuiTask::unlock();
		return;
	}

	if (current_theme != theme) {
		ESP_LOGI("ThemeEngine", "Setting theme to: %d", (int)theme);
		current_theme = theme;
		apply_theme(theme, disp);
	}
	GuiTask::unlock();
}

ThemeType ThemeEngine::get_current_theme() { return current_theme; }

void ThemeEngine::cycle_theme() {
	ThemeType next = current_theme;
	switch (next) {
		case ThemeType::HYPRLAND:
			next = ThemeType::MATERIAL;
			break;
		case ThemeType::MATERIAL:
			next = ThemeType::HYPRLAND;
			break;
	}
	ESP_LOGI("ThemeEngine", "Cycling theme to: %d", (int)next);
	GuiTask::lock();
	lv_subject_set_int(&System::SystemManager::getInstance().getThemeSubject(), (int32_t)next);
	GuiTask::unlock();
}

void ThemeEngine::cleanup_previous_theme(lv_display_t* disp) {
	ESP_LOGD("ThemeEngine", "Cleaning up previous theme for display %p", disp);
	if (engine_themes.count(disp)) {
		auto& themes = engine_themes[disp];
		for (auto it = themes.rbegin(); it != themes.rend(); ++it) {
			if (*it == lv_theme_default_get())
				continue;
			lv_theme_delete(*it);
		}
		themes.clear();
		engine_themes.erase(disp);
	}
}

void ThemeEngine::apply_theme(ThemeType theme, lv_display_t* disp) {
	GuiTask::lock();
	if (!disp)
		disp = lv_display_get_default();
	if (!disp) {
		GuiTask::unlock();
		return;
	}

	ESP_LOGI("ThemeEngine", "Applying theme %d to display %p", (int)theme, disp);
	ThemeConfig cfg = Themes::GetConfig(theme);
	std::vector<lv_theme_t*> new_themes_vec;

	lv_theme_t* base_th = lv_theme_default_init(disp, cfg.primary, cfg.secondary, cfg.dark, LV_FONT_DEFAULT);
	new_themes_vec.push_back(base_th);

	lv_theme_t* global_th = lv_theme_create();
	lv_theme_set_parent(global_th, base_th);
	lv_theme_set_apply_cb(global_th, Themes::ApplyGlobal);
	new_themes_vec.push_back(global_th);

	lv_theme_t* final_theme = global_th;
	if (cfg.apply_cb) {
		lv_theme_t* specific_th = lv_theme_create();
		lv_theme_set_parent(specific_th, global_th);
		lv_theme_set_apply_cb(specific_th, cfg.apply_cb);
		new_themes_vec.push_back(specific_th);
		final_theme = specific_th;
	}

	lv_display_set_theme(disp, final_theme);

	cleanup_previous_theme(disp);

	engine_themes[disp] = new_themes_vec;

	lv_obj_report_style_change(NULL);
	GuiTask::unlock();
}
