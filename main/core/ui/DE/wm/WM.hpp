#pragma once

#include <map>
#include <string>
#include <vector>

#include "core/apps/AppManager.hpp"
#include "lvgl.h"

class WM : public System::Apps::AppStateObserver {
public:

	static WM& getInstance();

	void init(lv_obj_t* window_container, lv_obj_t* app_container, lv_obj_t* screen, lv_obj_t* status_bar, lv_obj_t* dock);

	void openApp(const std::string& packageName);
	void closeApp(const std::string& packageName);
	void closeWindow(lv_obj_t* w);

	// AppStateObserver interface
	void onAppStarted(const std::string& packageName) override;
	void onAppStopped(const std::string& packageName) override;

	// State validation
	bool hasWindowForApp(const std::string& packageName) const;

private:

	WM();
	~WM();

	lv_obj_t* m_windowContainer;
	lv_obj_t* m_appContainer;
	lv_obj_t* m_screen;
	lv_obj_t* m_statusBar;
	lv_obj_t* m_dock;

	std::map<lv_obj_t*, std::string> m_windowAppMap;
	std::map<lv_obj_t*, lv_obj_t*> m_windowMaxBtnLabelMap;
	std::vector<lv_obj_t*> m_tiledWindows;
	lv_obj_t* m_fullScreenWindow = nullptr;

	void updateLayout();

	void toggleFullScreen(lv_obj_t* win);

	static void on_win_close(lv_event_t* e);
	static void on_win_focus(lv_event_t* e);
	static void on_win_minimize(lv_event_t* e);
	static void on_header_minimize(lv_event_t* e);
	static void on_win_maximize(lv_event_t* e);
	static void on_rotation_change(lv_observer_t* observer, lv_subject_t* subject);
	static void on_global_event(lv_event_t* e);

	// Window management helpers
	void closeWindow_internal(lv_obj_t* w);
	static void activate_window(lv_obj_t* target_win);
};
