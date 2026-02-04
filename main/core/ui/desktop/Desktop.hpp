#pragma once

#include "lvgl.h"
#include "modules/dock/Dock.hpp"
#include "modules/launcher/Launcher.hpp"
#include "modules/notification_panel/NotificationPanel.hpp"
#include "modules/quick_access_panel/QuickAccessPanel.hpp"
#include "modules/status_bar/StatusBar.hpp"
#include "modules/swipe_manager/SwipeManager.hpp"
#include <memory>
#include <string>

class Desktop {
public:

	static Desktop& getInstance();
	Desktop();
	~Desktop();

	void init();

	// These now delegate to WM
	void openApp(const std::string& packageName);
	void closeApp(const std::string& packageName);

private:

	lv_obj_t* m_screen {};
	lv_obj_t* m_wallpaper {};
	lv_obj_t* m_wallpaper_img {};
	lv_obj_t* m_wallpaper_icon {};
	lv_obj_t* m_window_container {};
	std::unique_ptr<UI::Modules::StatusBar> m_statusBarModule {};
	lv_obj_t* m_status_bar {};
	std::unique_ptr<UI::Modules::Dock> m_dockModule {};
	lv_obj_t* m_dock {};
	std::unique_ptr<UI::Modules::Launcher> m_launcherModule {};
	lv_obj_t* m_launcher {};
	std::unique_ptr<UI::Modules::QuickAccessPanel> m_quickAccessPanelModule {};
	lv_obj_t* m_quick_access_panel {};
	std::unique_ptr<UI::Modules::NotificationPanel> m_notificationPanelModule {};
	lv_obj_t* m_notification_panel {};
	lv_obj_t* m_notification_list {};
	lv_obj_t* m_clear_all_btn {};
	lv_obj_t* m_greetings {};
	lv_obj_t* m_app_container {};
	std::unique_ptr<UI::Modules::SwipeManager> m_swipeManagerModule {};

	void update_notification_list();
	void realign_panels();
	void configure_panel_style(lv_obj_t* panel);

	void on_start_click();
	void on_up_click();
	static void on_app_click(lv_event_t* e);
};
