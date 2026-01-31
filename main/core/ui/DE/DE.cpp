#include "DE.hpp"
#include "../theming/ThemeEngine.hpp"
#include "core/apps/AppManager.hpp"
#include "core/connectivity/ConnectivityManager.hpp"
#include "core/system/FocusManager.hpp"
#include "core/system/NotificationManager.hpp"
#include "core/system/SystemManager.hpp"

#include "esp_log.h"
#include "wm/WM.hpp"
#include <ctime>

static const char* TAG = "DE";

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
	  launcher(nullptr), quick_access_panel(nullptr), notification_panel(nullptr),
	  notification_list(nullptr), greetings(nullptr), app_container(nullptr) {}


DE::~DE() {}

void DE::init() {
	screen = lv_obj_create(NULL);
	lv_obj_remove_style_all(screen);
	lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(screen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	ESP_LOGI(TAG, "Initializing Desktop Environment...");
	lv_screen_load(screen);

	if (!System::SystemManager::getInstance().isSafeMode()) {
		ESP_LOGI(TAG, "Standard mode: Creating wallpaper and panels");
		wallpaper = lv_obj_create(screen);
		lv_obj_remove_style_all(wallpaper);
		lv_obj_set_size(wallpaper, lv_pct(100), lv_pct(100));

		ThemeConfig cfg = Themes::GetConfig(ThemeEngine::get_current_theme());
		lv_obj_set_style_bg_color(wallpaper, cfg.primary, 0);
		lv_obj_set_style_bg_opa(wallpaper, LV_OPA_COVER, 0);
		lv_obj_add_flag(wallpaper, LV_OBJ_FLAG_FLOATING);
		lv_obj_add_flag(wallpaper, LV_OBJ_FLAG_CLICK_FOCUSABLE); // Make focusable for panel dismissal
		lv_obj_move_background(wallpaper);

		wallpaper_icon = lv_image_create(wallpaper);
		lv_image_set_src(wallpaper_icon, LV_SYMBOL_IMAGE);
		lv_obj_set_style_text_opa(wallpaper_icon, LV_OPA_30, 0);
		lv_obj_set_style_text_font(wallpaper_icon, &lv_font_montserrat_14, 0); // lv_font_montserrat_48 looks good
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
						lv_image_set_src(instance->wallpaper_img, "A:/data/wallpaper.jpg");
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
		ESP_LOGW(TAG, "Safe mode detected");
		lv_obj_set_style_bg_color(screen, lv_palette_main(LV_PALETTE_GREY), 0);
		lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
	}

	ESP_LOGD(TAG, "Creating UI components...");
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
	create_notification_panel();

	// Register panels with FocusManager
	System::FocusManager::getInstance().registerPanel(launcher);
	System::FocusManager::getInstance().registerPanel(quick_access_panel);
	System::FocusManager::getInstance().registerPanel(notification_panel);


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
	ESP_LOGI(TAG, "Desktop Environment initialized.");
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
		ESP_LOGD(TAG, "UI component: Launcher app button created for '%s'", app->getAppName().c_str());

		lv_obj_set_user_data(btn, app.get());
	}
}

void DE::create_quick_access_panel() {
	ESP_LOGD(TAG, "Creating quick access panel");
	quick_access_panel = lv_obj_create(screen);
	configure_panel_style(quick_access_panel);
	lv_obj_align_to(quick_access_panel, dock, LV_ALIGN_OUT_TOP_RIGHT, 0, -lv_dpx(2));
	lv_obj_set_flex_flow(quick_access_panel, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(quick_access_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	ESP_LOGD(TAG, "UI component: Quick Access Panel created");

	lv_obj_t* label = lv_label_create(quick_access_panel);
	lv_label_set_text(label, "Quick Access");
	lv_obj_set_width(label, lv_pct(100));
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
	ESP_LOGD(TAG, "UI component: Quick Access Panel title label created");

	lv_obj_t* toggles_cont = lv_obj_create(quick_access_panel);
	lv_obj_remove_style_all(toggles_cont);
	lv_obj_set_size(toggles_cont, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(toggles_cont, LV_FLEX_FLOW_ROW_WRAP);
	lv_obj_set_flex_align(toggles_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	ESP_LOGD(TAG, "UI component: Quick Access Panel toggles container created");

	lv_obj_t* theme_cont = lv_obj_create(toggles_cont);
	lv_obj_remove_style_all(theme_cont);
	lv_obj_set_size(theme_cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(theme_cont, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(theme_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	ESP_LOGD(TAG, "UI component: Quick Access Panel theme container created");

	lv_obj_t* theme_btn = lv_button_create(theme_cont);
	lv_obj_set_size(theme_btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_style_radius(theme_btn, LV_RADIUS_CIRCLE, 0);
	lv_obj_t* theme_icon = lv_image_create(theme_btn);
	lv_image_set_src(theme_icon, LV_SYMBOL_IMAGE);
	lv_obj_center(theme_icon);
	ESP_LOGD(TAG, "UI component: Quick Access Panel theme button created");

	theme_label = lv_label_create(theme_cont);
	lv_label_set_text(theme_label, Themes::ToString(ThemeEngine::get_current_theme()));
	ESP_LOGD(TAG, "UI component: Quick Access Panel theme label created");

	lv_subject_add_observer_obj(
		&System::SystemManager::getInstance().getThemeSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			lv_obj_t* label = lv_observer_get_target_obj(observer);
			if (label) {
				int32_t v = lv_subject_get_int(subject);
				lv_label_set_text(label, Themes::ToString((ThemeType)v));
				ESP_LOGD(TAG, "Quick Access Panel: Theme label updated to %s", Themes::ToString((ThemeType)v));
			}
		},
		theme_label, nullptr
	);

	lv_obj_add_subject_toggle_event(
		theme_btn, &System::SystemManager::getInstance().getThemeSubject(),
		LV_EVENT_CLICKED
	);
	ESP_LOGD(TAG, "Event: Theme toggle event added to theme button");

	lv_obj_t* rot_cont = lv_obj_create(toggles_cont);
	lv_obj_remove_style_all(rot_cont);
	lv_obj_set_size(rot_cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(rot_cont, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(rot_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	ESP_LOGD(TAG, "UI component: Quick Access Panel rotation container created");

	lv_obj_t* rot_btn = lv_button_create(rot_cont);
	lv_obj_set_size(rot_btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_style_radius(rot_btn, LV_RADIUS_CIRCLE, 0);
	lv_obj_t* rot_icon = lv_image_create(rot_btn);
	lv_image_set_src(rot_icon, LV_SYMBOL_REFRESH);
	lv_obj_center(rot_icon);
	ESP_LOGD(TAG, "UI component: Quick Access Panel rotation button created");

	lv_obj_t* rot_label = lv_label_create(rot_cont);
	lv_label_bind_text(rot_label, &System::SystemManager::getInstance().getRotationSubject(), "%d°");
	ESP_LOGD(TAG, "UI component: Quick Access Panel rotation label created");

	lv_subject_increment_dsc_t* rot_dsc = lv_obj_add_subject_increment_event(
		rot_btn, &System::SystemManager::getInstance().getRotationSubject(),
		LV_EVENT_CLICKED, 90
	);
	lv_obj_set_subject_increment_event_min_value(rot_btn, rot_dsc, 0);
	lv_obj_set_subject_increment_event_max_value(rot_btn, rot_dsc, 270);
	lv_obj_set_subject_increment_event_rollover(rot_btn, rot_dsc, true);
	ESP_LOGD(TAG, "Event: Rotation increment event added to rotation button");

	{
		lv_obj_t* slider_cont = lv_obj_create(quick_access_panel);
		lv_obj_remove_style_all(slider_cont);
		lv_obj_set_size(slider_cont, lv_pct(100), LV_SIZE_CONTENT);
		lv_obj_set_flex_flow(slider_cont, LV_FLEX_FLOW_ROW);
		lv_obj_set_flex_align(slider_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		ESP_LOGD(TAG, "UI component: Quick Access Panel slider container created");

		lv_obj_t* icon = lv_image_create(slider_cont);
		lv_image_set_src(icon, LV_SYMBOL_SETTINGS);
		ESP_LOGD(TAG, "UI component: Quick Access Panel slider icon created");

		lv_obj_t* slider = lv_slider_create(slider_cont);
		lv_obj_set_flex_grow(slider, 1);
		lv_obj_set_height(slider, lv_pct(70));
		lv_slider_set_range(slider, 0, 255);
		lv_slider_bind_value(
			slider, &System::SystemManager::getInstance().getBrightnessSubject()
		);
		ESP_LOGD(TAG, "UI component: Quick Access Panel brightness slider created");
	}
}

void DE::realign_panels() {
	if (dock) {
		if (launcher) {
			lv_obj_align_to(launcher, dock, LV_ALIGN_OUT_TOP_LEFT, 0, -lv_dpx(2));
			ESP_LOGD(TAG, "Realigned launcher panel");
		}
		if (quick_access_panel) {
			lv_obj_align_to(quick_access_panel, dock, LV_ALIGN_OUT_TOP_RIGHT, 0, -lv_dpx(2));
			ESP_LOGD(TAG, "Realigned quick access panel");
		}
		if (greetings) {
			lv_obj_align_to(greetings, dock, LV_ALIGN_OUT_TOP_RIGHT, -lv_dpx(2), -lv_dpx(2));
		}
		if (notification_panel) {
			lv_obj_align(notification_panel, LV_ALIGN_TOP_MID, 0, lv_dpx(30));
		}
	}
}

void DE::on_start_click(lv_event_t* e) {
	DE* d = (DE*)lv_event_get_user_data(e);
	if (d && d->launcher) {
		d->realign_panels();
		System::FocusManager::getInstance().togglePanel(d->launcher);
	}
}

void DE::on_up_click(lv_event_t* e) {
	DE* d = (DE*)lv_event_get_user_data(e);
	if (d && d->quick_access_panel) {
		d->realign_panels();
		System::FocusManager::getInstance().togglePanel(d->quick_access_panel);
	}
}


void DE::on_app_click(lv_event_t* e) {
	DE* d = (DE*)lv_event_get_user_data(e);
	lv_obj_t* btn = lv_event_get_target_obj(e);


	System::Apps::App* appPtr = (System::Apps::App*)lv_obj_get_user_data(btn);
	if (!appPtr)
		return;

	std::string packageName = appPtr->getPackageName();

	ESP_LOGI(TAG, "App clicked in launcher: %s", packageName.c_str());
	System::FocusManager::getInstance().dismissAllPanels();
	d->openApp(packageName);
}

void DE::openApp(const std::string& packageName) {
	ESP_LOGI(TAG, "Opening App: %s", packageName.c_str());
	WM::getInstance().openApp(packageName);
}

void DE::closeApp(const std::string& packageName) {
	ESP_LOGI(TAG, "Closing App: %s", packageName.c_str());
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

	lv_obj_t* wifi_icon = lv_image_create(left_group);
	lv_image_set_src(wifi_icon, LV_SYMBOL_WIFI);
	lv_subject_add_observer_obj(
		&System::ConnectivityManager::getInstance().getWiFiConnectedSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			lv_obj_t* obj = lv_observer_get_target_obj(observer);
			if (lv_subject_get_int(subject)) {
				lv_obj_set_style_image_opa(obj, LV_OPA_COVER, 0);
			} else {
				lv_obj_set_style_image_opa(obj, LV_OPA_40, 0);
			}
		},
		wifi_icon, nullptr
	);

	lv_obj_t* hotspot_group = lv_obj_create(left_group);
	lv_obj_remove_style_all(hotspot_group);
	lv_obj_set_size(hotspot_group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(hotspot_group, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(hotspot_group, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_t* hotspot_icon = lv_image_create(hotspot_group);
	lv_image_set_src(hotspot_icon, LV_SYMBOL_WIFI);

	lv_obj_t* hotspot_label = lv_label_create(hotspot_group);

	lv_subject_add_observer_obj(
		&System::ConnectivityManager::getInstance().getHotspotEnabledSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			lv_obj_t* group = lv_observer_get_target_obj(observer);
			if (lv_subject_get_int(subject)) {
				lv_obj_set_style_image_opa(group, LV_OPA_COVER, 0);
			} else {
				lv_obj_set_style_image_opa(group, LV_OPA_40, 0);
			}
		},
		hotspot_group, nullptr
	);

	lv_subject_add_observer_obj(
		&System::ConnectivityManager::getInstance().getHotspotClientsSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			lv_obj_t* label = lv_observer_get_target_obj(observer);
			int32_t clients = lv_subject_get_int(subject);
			if (clients > 0) {
				lv_label_set_text_fmt(label, "%d", (int)clients);
			} else {
				lv_label_set_text(label, "");
			}
		},
		hotspot_label, nullptr
	);

	lv_obj_t* bt_icon = lv_image_create(left_group);
	lv_image_set_src(bt_icon, LV_SYMBOL_BLUETOOTH);
	lv_subject_add_observer_obj(
		&System::ConnectivityManager::getInstance().getBluetoothEnabledSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			lv_obj_t* icon = lv_observer_get_target_obj(observer);
			if (lv_subject_get_int(subject)) {
				lv_obj_set_style_image_opa(icon, LV_OPA_COVER, 0);
			} else {
				lv_obj_set_style_image_opa(icon, LV_OPA_40, 0);
			}
		},
		bt_icon, nullptr
	);

	lv_obj_t* notif_btn = lv_obj_create(left_group);
	lv_obj_remove_style_all(notif_btn);
	lv_obj_set_size(notif_btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(notif_btn, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(notif_btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_add_event_cb(notif_btn, on_notification_click, LV_EVENT_CLICKED, this);

	lv_obj_t* notif_icon = lv_image_create(notif_btn);
	lv_image_set_src(notif_icon, LV_SYMBOL_BELL);

	lv_obj_t* notif_badge = lv_label_create(notif_btn);
	lv_obj_set_style_text_font(notif_badge, &lv_font_montserrat_14, 0);


	lv_subject_add_observer_obj(
		&System::NotificationManager::getInstance().getUnreadCountSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			lv_obj_t* btn = lv_observer_get_target_obj(observer);
			lv_obj_t* badge = lv_obj_get_child(btn, 1);
			lv_obj_t* icon = lv_obj_get_child(btn, 0);

			int32_t count = lv_subject_get_int(subject);
			if (count > 0) {
				lv_label_set_text_fmt(badge, "%d", (int)count);
				lv_obj_set_style_image_opa(icon, LV_OPA_COVER, 0);
			} else {
				lv_label_set_text(badge, "");
				lv_obj_set_style_image_opa(icon, LV_OPA_40, 0);
			}
		},
		notif_btn, nullptr
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

void DE::create_notification_panel() {
	ESP_LOGD(TAG, "Creating notification panel");
	notification_panel = lv_obj_create(screen);
	configure_panel_style(notification_panel);
	// Center top, slightly below status bar
	lv_obj_set_size(notification_panel, lv_pct(50), lv_pct(60));
	lv_obj_align(notification_panel, LV_ALIGN_TOP_MID, 0, lv_dpx(30));
	lv_obj_set_flex_flow(notification_panel, LV_FLEX_FLOW_COLUMN);

	lv_obj_t* header = lv_obj_create(notification_panel);
	lv_obj_remove_style_all(header);
	lv_obj_set_size(header, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_all(header, lv_dpx(10), 0);

	lv_obj_t* title = lv_label_create(header);
	lv_label_set_text(title, "Notifications");
	lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);

	lv_obj_t* clear_btn = lv_button_create(header);
	lv_obj_set_size(clear_btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_style_pad_all(clear_btn, lv_dpx(5), 0);
	lv_obj_t* clear_label = lv_label_create(clear_btn);
	lv_label_set_text(clear_label, "Clear All");
	lv_obj_add_event_cb(clear_btn, on_clear_notifications_click, LV_EVENT_CLICKED, this);

	notification_list = lv_obj_create(notification_panel);
	lv_obj_remove_style_all(notification_list);
	lv_obj_set_width(notification_list, lv_pct(100));
	lv_obj_set_flex_grow(notification_list, 1);

	lv_obj_set_flex_flow(notification_list, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_all(notification_list, lv_dpx(5), 0);
	lv_obj_set_style_pad_row(notification_list, lv_dpx(5), 0);

	// Observe updates
	lv_subject_add_observer_obj(
		&System::NotificationManager::getInstance().getUpdateSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			DE* instance = (DE*)lv_observer_get_user_data(observer);
			if (instance) {
				instance->update_notification_list();
			}
		},
		notification_list, this
	);

	update_notification_list();
}

void DE::update_notification_list() {
	if (!notification_list) return;

	lv_obj_clean(notification_list);

	const auto& notifs = System::NotificationManager::getInstance().getNotifications();

	if (notifs.empty()) {
		lv_obj_t* empty_label = lv_label_create(notification_list);
		lv_label_set_text(empty_label, "No new notifications");
		lv_obj_center(empty_label);
		lv_obj_set_style_text_opa(empty_label, LV_OPA_50, 0);
		return;
	}

	for (const auto& n: notifs) {
		lv_obj_t* item = lv_obj_create(notification_list);
		lv_obj_set_size(item, lv_pct(100), LV_SIZE_CONTENT);
		lv_obj_set_style_radius(item, lv_dpx(8), 0);
		lv_obj_set_style_bg_opa(item, LV_OPA_20, 0); // Very transparent
		lv_obj_set_flex_flow(item, LV_FLEX_FLOW_ROW);
		lv_obj_set_flex_align(item, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		lv_obj_set_style_pad_all(item, lv_dpx(8), 0);

		if (n.icon) {
			lv_obj_t* icon = lv_image_create(item);
			lv_image_set_src(icon, n.icon);
		}

		lv_obj_t* content = lv_obj_create(item);
		lv_obj_remove_style_all(content);
		lv_obj_set_flex_grow(content, 1);
		lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);

		lv_obj_t* title_lbl = lv_label_create(content);
		lv_label_set_text(title_lbl, n.title.c_str());
		lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_14, 0);

		lv_obj_t* msg_lbl = lv_label_create(content);
		lv_label_set_text(msg_lbl, n.message.c_str());
		lv_label_set_long_mode(msg_lbl, LV_LABEL_LONG_WRAP);
		lv_obj_set_width(msg_lbl, lv_pct(95));
		lv_obj_set_style_text_opa(msg_lbl, LV_OPA_70, 0);

		lv_obj_t* time_lbl = lv_label_create(item);
		// Simple timestamp logic, could be better
		lv_label_set_text(time_lbl, "now");
		lv_obj_set_style_text_opa(time_lbl, LV_OPA_50, 0);
		lv_obj_set_style_text_font(time_lbl, &lv_font_montserrat_14, 0);
	}
}

void DE::on_notification_click(lv_event_t* e) {
	DE* d = (DE*)lv_event_get_user_data(e);
	if (d && d->notification_panel) {
		d->realign_panels();
		System::FocusManager::getInstance().togglePanel(d->notification_panel);
	}
}

void DE::on_clear_notifications_click(lv_event_t* e) {
	System::NotificationManager::getInstance().clearAll();
}
