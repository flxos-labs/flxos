#pragma once

#include "lvgl.h"
#include <string>

class DE {
public:

	static DE& getInstance();
	DE();
	~DE();

	void init();

	// These now delegate to WM
	void openApp(const std::string& packageName);
	void closeApp(const std::string& packageName);

	// UI Utilities
	static void apply_glass(lv_obj_t* obj, int32_t blur);
	static lv_obj_t* create_dock_btn(lv_obj_t* parent, const char* icon, int32_t w, int32_t h);

private:

	lv_obj_t* screen;
	lv_obj_t* wallpaper;
	lv_obj_t* wallpaper_img;
	lv_obj_t* wallpaper_icon;
	lv_obj_t* window_container;
	lv_obj_t* status_bar;
	lv_obj_t* dock;
	lv_obj_t* time_label;
	lv_obj_t* theme_label;
	lv_obj_t* launcher;
	lv_obj_t* quick_access_panel;
	lv_obj_t* notification_panel;
	lv_obj_t* notification_list;
	lv_obj_t* greetings;
	lv_obj_t* app_container;
	lv_obj_t* swipe_trigger_zone;

	// Swipe tracking state
	lv_coord_t swipe_start_y;
	bool swipe_active;

	void create_status_bar();
	void create_dock();
	void create_launcher();
	void create_quick_access_panel();
	void create_notification_panel();
	void update_notification_list();
	void realign_panels();
	void configure_panel_style(lv_obj_t* panel);
	void create_swipe_trigger_zone();

	static void on_start_click(lv_event_t* e);
	static void on_up_click(lv_event_t* e);
	static void on_clear_notifications_click(lv_event_t* e);
	static void on_app_click(lv_event_t* e);
	static void on_swipe_zone_press(lv_event_t* e);
	static void on_swipe_zone_pressing(lv_event_t* e);
	static void on_swipe_zone_release(lv_event_t* e);
	static void on_notif_panel_press(lv_event_t* e);
	static void on_notif_panel_pressing(lv_event_t* e);
	static void on_notif_panel_release(lv_event_t* e);
};
