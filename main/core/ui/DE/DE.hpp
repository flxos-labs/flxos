#pragma once

#include "lvgl.h"
#include "modules/Dock/Dock.hpp"
#include "modules/Launcher/Launcher.hpp"
#include "modules/NotificationPanel/NotificationPanel.hpp"
#include "modules/QuickAccessPanel/QuickAccessPanel.hpp"
#include "modules/StatusBar/StatusBar.hpp"
#include "modules/SwipeManager/SwipeManager.hpp"
#include <memory>
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

private:

	lv_obj_t* screen;
	lv_obj_t* wallpaper;
	lv_obj_t* wallpaper_img;
	lv_obj_t* wallpaper_icon;
	lv_obj_t* window_container;
	std::unique_ptr<UI::Modules::StatusBar> m_statusBarModule;
	lv_obj_t* status_bar;
	std::unique_ptr<UI::Modules::Dock> m_dockModule;
	lv_obj_t* dock;
	std::unique_ptr<UI::Modules::Launcher> m_launcherModule;
	lv_obj_t* launcher;
	std::unique_ptr<UI::Modules::QuickAccessPanel> m_quickAccessPanelModule;
	lv_obj_t* quick_access_panel;
	std::unique_ptr<UI::Modules::NotificationPanel> m_notificationPanelModule;
	lv_obj_t* notification_panel;
	lv_obj_t* notification_list;
	lv_obj_t* clear_all_btn;
	lv_obj_t* greetings;
	lv_obj_t* app_container;
	std::unique_ptr<UI::Modules::SwipeManager> m_swipeManagerModule;

	void update_notification_list();
	void realign_panels();
	void configure_panel_style(lv_obj_t* panel);

	void on_start_click();
	void on_up_click();
	static void on_app_click(lv_event_t* e);
};
