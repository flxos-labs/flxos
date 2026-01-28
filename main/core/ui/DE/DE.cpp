#include "DE.hpp"
#include "../theming/ThemeEngine.hpp"
#include "core/apps/AppManager.hpp"
#include "core/connectivity/ConnectivityManager.hpp"
#include "core/system/SystemManager.hpp"
#include "esp_timer.h"
#include "src/drivers/display/lovyan_gfx/lv_lovyan_gfx.h"
#include "wm/WM.hpp"
#include <algorithm>
#include <cstring>
#include <ctime>
#include <vector>

DE& DE::getInstance() {
	static DE instance;
	return instance;
}

void DE::apply_glass(lv_obj_t* obj, int32_t blur) {
	lv_obj_set_style_bg_opa(obj, LV_OPA_60, 0);

	lv_subject_add_observer_obj(
		&System::SystemManager::getInstance().getGlassEnabledSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			lv_obj_t* target = lv_observer_get_target_obj(observer);
			int32_t b = (intptr_t)lv_observer_get_user_data(observer);
			bool enabled = lv_subject_get_int(subject);
			if (enabled) {
				lv_obj_set_style_blur_backdrop(target, true, 0);
				lv_obj_set_style_blur_radius(target, b, 0);
			} else {
				lv_obj_set_style_blur_backdrop(target, false, 0);
				lv_obj_set_style_blur_radius(target, 0, 0);
			}
		},
		obj, (void*)(intptr_t)blur
	);
}

lv_obj_t* DE::create_dock_btn(lv_obj_t* parent, const char* icon, int32_t w, int32_t h) {
	lv_obj_t* btn = lv_button_create(parent);
	lv_obj_set_size(btn, w, h);
	lv_obj_set_style_radius(btn, lv_dpx(6), 0);
	lv_obj_t* img = lv_image_create(btn);
	lv_image_set_src(img, icon);
	lv_obj_center(img);
	return btn;
}

DE::DE()
	: screen(nullptr), wallpaper(nullptr), wallpaper_img(nullptr),
	  wallpaper_icon(nullptr), window_container(nullptr), status_bar(nullptr),
	  dock(nullptr), time_label(nullptr), theme_label(nullptr),
	  launcher(nullptr), quick_access_panel(nullptr), greetings(nullptr),
	  app_container(nullptr) {}

DE::~DE() {}

void DE::init() {
	screen = lv_obj_create(NULL);
	lv_obj_remove_style_all(screen);
	lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_screen_load(screen);

	if (!System::SystemManager::getInstance().isSafeMode()) {
		wallpaper = lv_obj_create(screen);
		lv_obj_remove_style_all(wallpaper);
		lv_obj_set_size(wallpaper, lv_pct(100), lv_pct(100));

		ThemeConfig cfg = Themes::GetConfig(ThemeEngine::get_current_theme());
		lv_obj_set_style_bg_color(wallpaper, cfg.primary, 0);
		lv_obj_set_style_bg_opa(wallpaper, LV_OPA_COVER, 0);
		lv_obj_add_flag(wallpaper, LV_OBJ_FLAG_FLOATING);
		lv_obj_move_background(wallpaper);

		wallpaper_icon = lv_label_create(wallpaper);
		lv_label_set_text(wallpaper_icon, LV_SYMBOL_IMAGE);
		lv_obj_set_style_text_font(wallpaper_icon, &lv_font_montserrat_14, 0);
		lv_obj_set_style_text_opa(wallpaper_icon, LV_OPA_30, 0);
		lv_obj_center(wallpaper_icon);

		lv_subject_add_observer_obj(
			&System::SystemManager::getInstance().getThemeSubject(),
			[](lv_observer_t* observer, lv_subject_t* subject) {
				lv_obj_t* wp = lv_observer_get_target_obj(observer);
				if (wp) {
					ThemeType theme = (ThemeType)lv_subject_get_int(subject);
					ThemeConfig cfg = Themes::GetConfig(theme);
					lv_obj_set_style_bg_color(wp, cfg.primary, 0);
				}
			},
			wallpaper, nullptr
		);

		lv_subject_add_observer(
			&System::SystemManager::getInstance().getWallpaperEnabledSubject(),
			[](lv_observer_t* observer, lv_subject_t* subject) {
				DE* instance = (DE*)lv_observer_get_user_data(observer);
				bool enabled = lv_subject_get_int(subject);

				if (enabled) {
					if (instance->wallpaper_icon)
						lv_obj_add_flag(instance->wallpaper_icon, LV_OBJ_FLAG_HIDDEN);
					if (instance->wallpaper_img == nullptr &&
						instance->wallpaper != nullptr) {
						instance->wallpaper_img = lv_image_create(instance->wallpaper);
						lv_image_set_src(instance->wallpaper_img, "A:/data/wallpaper.png");
						lv_obj_set_size(instance->wallpaper_img, lv_pct(100), lv_pct(100));
						lv_obj_set_style_pad_all(instance->wallpaper_img, 0, 0);
						lv_obj_set_style_border_width(instance->wallpaper_img, 0, 0);
						lv_image_set_inner_align(instance->wallpaper_img, LV_IMAGE_ALIGN_COVER);
						lv_obj_move_background(instance->wallpaper_img);
					}
				} else {
					if (instance->wallpaper_icon)
						lv_obj_clear_flag(instance->wallpaper_icon, LV_OBJ_FLAG_HIDDEN);
					if (instance->wallpaper_img != nullptr) {
						lv_obj_delete(instance->wallpaper_img);
						instance->wallpaper_img = nullptr;
					}
				}
			},
			this
		);
	} else {
		lv_obj_set_style_bg_color(screen, lv_palette_main(LV_PALETTE_GREY), 0);
		lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
	}

	create_status_bar();

	window_container = lv_obj_create(screen);
	lv_obj_remove_style_all(window_container);
	lv_obj_remove_flag(window_container, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_width(window_container, lv_pct(100));
	lv_obj_set_flex_grow(window_container, 1);

	create_dock();

	WM::getInstance().init(window_container, app_container, screen, status_bar, dock);

	if (wallpaper) {
		greetings = lv_label_create(wallpaper);
		lv_label_set_text(greetings, "Hey !");
		lv_obj_align_to(greetings, dock, LV_ALIGN_OUT_TOP_RIGHT, -lv_dpx(2), -lv_dpx(2));
	}

	create_launcher();
	create_quick_access_panel();

	lv_subject_add_observer(
		&System::SystemManager::getInstance().getRotationSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			DE* instance = (DE*)lv_observer_get_user_data(observer);
			if (instance && instance->screen) {
				lv_obj_update_layout(instance->screen);
				instance->realign_panels();
			}
		},
		this
	);
}

void DE::configure_panel_style(lv_obj_t* panel) {
	lv_obj_set_size(panel, lv_pct(80), lv_pct(60));
	lv_obj_set_style_pad_all(panel, 0, 0);
	lv_obj_set_style_radius(panel, lv_dpx(10), 0);
	lv_obj_set_style_border_width(panel, 0, 0);
	lv_obj_add_flag(panel, LV_OBJ_FLAG_FLOATING);
	lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);
	DE::apply_glass(panel, lv_dpx(4));
}

void DE::create_launcher() {
	launcher = lv_obj_create(screen);
	configure_panel_style(launcher);
	lv_obj_align_to(launcher, dock, LV_ALIGN_OUT_TOP_LEFT, 0, -lv_dpx(2));

	lv_obj_t* label = lv_label_create(launcher);
	lv_label_set_text(label, "Applications");
	lv_obj_align(label, LV_ALIGN_TOP_LEFT, lv_dpx(10), 0);

	lv_obj_t* list = lv_list_create(launcher);
	lv_obj_set_size(list, lv_pct(100), lv_pct(85));
	lv_obj_set_style_pad_all(list, 0, 0);
	lv_obj_align(list, LV_ALIGN_BOTTOM_MID, 0, 0);
	lv_obj_set_style_bg_opa(list, 0, 0);
	lv_obj_set_style_border_width(list, 0, 0);

	const auto& apps = System::Apps::AppManager::getInstance().getInstalledApps();
	for (const auto& app: apps) {
		lv_obj_t* btn = lv_list_add_button(list, NULL, app->getAppName().c_str());
		lv_obj_t* img = lv_image_create(btn);
		lv_image_set_src(img, app->getIcon());
		lv_obj_move_to_index(img, 0);

		lv_obj_add_event_cb(btn, on_app_click, LV_EVENT_CLICKED, this);

		lv_obj_set_user_data(btn, app.get());
	}
}

void DE::create_quick_access_panel() {
	quick_access_panel = lv_obj_create(screen);
	configure_panel_style(quick_access_panel);
	lv_obj_align_to(quick_access_panel, dock, LV_ALIGN_OUT_TOP_RIGHT, 0, -lv_dpx(2));
	lv_obj_set_flex_flow(quick_access_panel, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(quick_access_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_t* label = lv_label_create(quick_access_panel);
	lv_label_set_text(label, "Quick Access");
	lv_obj_set_width(label, lv_pct(100));
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);

	lv_obj_t* toggles_cont = lv_obj_create(quick_access_panel);
	lv_obj_remove_style_all(toggles_cont);
	lv_obj_set_size(toggles_cont, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(toggles_cont, LV_FLEX_FLOW_ROW_WRAP);
	lv_obj_set_flex_align(toggles_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_t* theme_cont = lv_obj_create(toggles_cont);
	lv_obj_remove_style_all(theme_cont);
	lv_obj_set_size(theme_cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(theme_cont, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(theme_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_t* theme_btn = lv_button_create(theme_cont);
	lv_obj_set_size(theme_btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_style_radius(theme_btn, LV_RADIUS_CIRCLE, 0);
	lv_obj_t* theme_icon = lv_image_create(theme_btn);
	lv_image_set_src(theme_icon, LV_SYMBOL_IMAGE);
	lv_obj_center(theme_icon);

	theme_label = lv_label_create(theme_cont);
	lv_label_set_text(theme_label, Themes::ToString(ThemeEngine::get_current_theme()));

	lv_subject_add_observer_obj(
		&System::SystemManager::getInstance().getThemeSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			lv_obj_t* label = lv_observer_get_target_obj(observer);
			if (label) {
				int32_t v = lv_subject_get_int(subject);
				lv_label_set_text(label, Themes::ToString((ThemeType)v));
			}
		},
		theme_label, nullptr
	);

	lv_obj_add_subject_toggle_event(
		theme_btn, &System::SystemManager::getInstance().getThemeSubject(),
		LV_EVENT_CLICKED
	);

	lv_obj_t* rot_cont = lv_obj_create(toggles_cont);
	lv_obj_remove_style_all(rot_cont);
	lv_obj_set_size(rot_cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(rot_cont, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(rot_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_t* rot_btn = lv_button_create(rot_cont);
	lv_obj_set_size(rot_btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_style_radius(rot_btn, LV_RADIUS_CIRCLE, 0);
	lv_obj_t* rot_icon = lv_image_create(rot_btn);
	lv_image_set_src(rot_icon, LV_SYMBOL_REFRESH);
	lv_obj_center(rot_icon);

	lv_obj_t* rot_label = lv_label_create(rot_cont);
	lv_label_bind_text(rot_label, &System::SystemManager::getInstance().getRotationSubject(), "%d°");

	lv_subject_increment_dsc_t* rot_dsc = lv_obj_add_subject_increment_event(
		rot_btn, &System::SystemManager::getInstance().getRotationSubject(),
		LV_EVENT_CLICKED, 90
	);
	lv_obj_set_subject_increment_event_min_value(rot_btn, rot_dsc, 0);
	lv_obj_set_subject_increment_event_max_value(rot_btn, rot_dsc, 270);
	lv_obj_set_subject_increment_event_rollover(rot_btn, rot_dsc, true);

	{
		lv_obj_t* slider_cont = lv_obj_create(quick_access_panel);
		lv_obj_remove_style_all(slider_cont);
		lv_obj_set_size(slider_cont, lv_pct(100), LV_SIZE_CONTENT);
		lv_obj_set_flex_flow(slider_cont, LV_FLEX_FLOW_ROW);
		lv_obj_set_flex_align(slider_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		lv_obj_t* icon = lv_image_create(slider_cont);
		lv_image_set_src(icon, LV_SYMBOL_SETTINGS);

		lv_obj_t* slider = lv_slider_create(slider_cont);
		lv_obj_set_flex_grow(slider, 1);
		lv_obj_set_height(slider, lv_pct(70));
		lv_slider_set_range(slider, 0, 255);
		lv_slider_bind_value(
			slider, &System::SystemManager::getInstance().getBrightnessSubject()
		);
	}
}

void DE::realign_panels() {
	if (dock) {
		if (launcher) {
			lv_obj_align_to(launcher, dock, LV_ALIGN_OUT_TOP_LEFT, 0, -lv_dpx(2));
		}
		if (quick_access_panel) {
			lv_obj_align_to(quick_access_panel, dock, LV_ALIGN_OUT_TOP_RIGHT, 0, -lv_dpx(2));
		}
		if (greetings) {
			lv_obj_align_to(greetings, dock, LV_ALIGN_OUT_TOP_RIGHT, -lv_dpx(2), -lv_dpx(2));
		}
	}
}

void DE::on_start_click(lv_event_t* e) {
	DE* d = (DE*)lv_event_get_user_data(e);
	if (lv_obj_has_flag(d->launcher, LV_OBJ_FLAG_HIDDEN)) {
		d->realign_panels();
		lv_obj_clear_flag(d->launcher, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(d->quick_access_panel, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_obj_add_flag(d->launcher, LV_OBJ_FLAG_HIDDEN);
	}
}

void DE::on_up_click(lv_event_t* e) {
	DE* d = (DE*)lv_event_get_user_data(e);
	if (lv_obj_has_flag(d->quick_access_panel, LV_OBJ_FLAG_HIDDEN)) {
		d->realign_panels();
		lv_obj_clear_flag(d->quick_access_panel, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(d->launcher, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_obj_add_flag(d->quick_access_panel, LV_OBJ_FLAG_HIDDEN);
	}
}

void DE::on_app_click(lv_event_t* e) {
	DE* d = (DE*)lv_event_get_user_data(e);
	lv_obj_t* btn = lv_event_get_target_obj(e);

	System::Apps::App* appPtr = (System::Apps::App*)lv_obj_get_user_data(btn);
	if (!appPtr)
		return;

	std::string packageName = appPtr->getPackageName();

	d->openApp(packageName);

	if (d && d->launcher) {
		lv_obj_add_flag(d->launcher, LV_OBJ_FLAG_HIDDEN);
	}
}

void DE::openApp(const std::string& packageName) {
	WM::getInstance().openApp(packageName);
}

void DE::closeApp(const std::string& packageName) {
	WM::getInstance().closeApp(packageName);
}

void DE::create_status_bar() {
	status_bar = lv_obj_create(screen);
	lv_obj_remove_style_all(status_bar);
	lv_obj_set_size(status_bar, lv_pct(100), lv_pct(7));
	lv_obj_set_style_pad_hor(status_bar, lv_dpx(4), 0);
	lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);

	DE::apply_glass(status_bar, lv_dpx(10));

	lv_obj_set_flex_flow(status_bar, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(status_bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_t* left_group = lv_obj_create(status_bar);
	lv_obj_remove_style_all(left_group);
	lv_obj_set_size(left_group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_flex_grow(left_group, 1);
	lv_obj_set_flex_flow(left_group, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(left_group, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	if (System::SystemManager::getInstance().isSafeMode()) {
		lv_obj_t* safe_img = lv_image_create(left_group);
		lv_image_set_src(safe_img, LV_SYMBOL_WARNING);
		lv_obj_set_style_image_recolor(safe_img, lv_palette_main(LV_PALETTE_RED), 0);
		lv_obj_set_style_image_recolor_opa(safe_img, LV_OPA_COVER, 0);

		lv_obj_t* safe_label = lv_label_create(left_group);
		lv_label_set_text(safe_label, " SAFE MODE");
	}

	lv_obj_t* wifi_icon = lv_label_create(left_group);
	lv_label_set_text(wifi_icon, LV_SYMBOL_WIFI);
	lv_subject_add_observer_obj(
		&System::ConnectivityManager::getInstance().getWiFiConnectedSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			lv_obj_t* obj = lv_observer_get_target_obj(observer);
			if (lv_subject_get_int(subject)) {
				lv_obj_set_style_text_opa(obj, LV_OPA_COVER, 0);
			} else {
				lv_obj_set_style_text_opa(obj, LV_OPA_40, 0);
			}
		},
		wifi_icon, nullptr
	);

	time_label = lv_label_create(status_bar);

	time_t now;
	struct tm timeinfo;
	time(&now);
	localtime_r(&now, &timeinfo);
	lv_label_set_text_fmt(time_label, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);

	lv_timer_create(
		[](lv_timer_t* t) {
			lv_obj_t* label = (lv_obj_t*)lv_timer_get_user_data(t);
			time_t now;
			struct tm timeinfo;
			time(&now);
			localtime_r(&now, &timeinfo);
			lv_label_set_text_fmt(label, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
		},
		1000, time_label
	);
}

void DE::create_dock() {
	dock = lv_obj_create(screen);
	lv_obj_remove_style_all(dock);
	lv_obj_set_size(dock, lv_pct(90), lv_pct(14));
	lv_obj_set_style_pad_hor(dock, lv_dpx(4), 0);
	lv_obj_set_style_radius(dock, lv_dpx(8), 0);
	lv_obj_set_style_margin_bottom(dock, lv_dpx(4), 0);

	DE::apply_glass(dock, lv_dpx(15));

	lv_obj_set_flex_flow(dock, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(dock, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_add_event_cb(
		DE::create_dock_btn(dock, LV_SYMBOL_LIST, lv_pct(13), lv_pct(85)),
		on_start_click, LV_EVENT_CLICKED, this
	);

	app_container = lv_obj_create(dock);
	lv_obj_remove_style_all(app_container);
	lv_obj_set_size(app_container, 0, lv_pct(100));
	lv_obj_set_style_pad_hor(app_container, lv_dpx(4), 0);
	lv_obj_set_flex_grow(app_container, 1);
	lv_obj_set_flex_flow(app_container, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(app_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_add_event_cb(
		DE::create_dock_btn(dock, LV_SYMBOL_UP, lv_pct(13), lv_pct(80)),
		on_up_click, LV_EVENT_CLICKED, this
	);
}