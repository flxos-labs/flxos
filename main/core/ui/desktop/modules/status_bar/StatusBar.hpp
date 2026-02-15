#pragma once
#include "core/ui/LvglObserverBridge.hpp"
#include "lvgl.h"
#include <memory>

namespace UI::Modules {

class StatusBar {
public:

	explicit StatusBar(lv_obj_t* parent);
	~StatusBar();

	lv_obj_t* getObj() const { return m_statusBar; }

	/// Show a temporary text overlay on the left side of the status bar
	static void showOverlay(const char* text);
	/// Remove the overlay text from the status bar
	static void clearOverlay();

private:

	void create();

	lv_obj_t* m_parent;
	lv_obj_t* m_statusBar = nullptr;
	lv_obj_t* m_timeLabel = nullptr;
	lv_timer_t* m_timer = nullptr;

	std::unique_ptr<System::LvglObserverBridge<int32_t>> m_wifiConnectedBridge;
	std::unique_ptr<System::LvglObserverBridge<int32_t>> m_wifiEnabledBridge;
	std::unique_ptr<System::LvglObserverBridge<int32_t>> m_hotspotEnabledBridge;
	std::unique_ptr<System::LvglObserverBridge<int32_t>> m_hotspotClientsBridge;
	std::unique_ptr<System::LvglObserverBridge<int32_t>> m_bluetoothEnabledBridge;

	static lv_obj_t* s_overlayLabel;
	static lv_obj_t* s_statusBarInstance;
};

} // namespace UI::Modules
