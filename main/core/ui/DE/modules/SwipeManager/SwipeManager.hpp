#pragma once
#include "lvgl.h"

namespace UI::Modules {

class SwipeManager {
public:

	struct Config {
		lv_obj_t* screen;
		lv_obj_t* statusBar;
		lv_obj_t* notificationPanel;
		lv_obj_t* notificationList;
	};

	SwipeManager(const Config& config);
	~SwipeManager() = default;

private:

	void create_trigger_zone();

	static void on_swipe_zone_press(lv_event_t* e);
	static void on_swipe_zone_pressing(lv_event_t* e);
	static void on_swipe_zone_release(lv_event_t* e);

	static void on_notif_panel_press(lv_event_t* e);
	static void on_notif_panel_pressing(lv_event_t* e);
	static void on_notif_panel_release(lv_event_t* e);

	Config m_config;
	lv_obj_t* m_triggerZone = nullptr;
	int32_t m_swipeStartY = 0;
	bool m_swipeActive = false;
};

} // namespace UI::Modules
