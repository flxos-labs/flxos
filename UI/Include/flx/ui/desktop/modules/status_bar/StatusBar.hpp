#pragma once
#include "lvgl.h"
#include <flx/ui/LvglObserverBridge.hpp>
#include <memory>

namespace UI::Modules {

class StatusBar {
public:

	explicit StatusBar(lv_obj_t* parent);
	~StatusBar();

	lv_obj_t* getObj() const { return m_statusBar; }

private:

	void create();

	lv_obj_t* m_parent;
	lv_obj_t* m_statusBar = nullptr;
	lv_obj_t* m_timeLabel = nullptr;
	lv_timer_t* m_timer = nullptr;

	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_wifiConnectedBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_wifiEnabledBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_hotspotEnabledBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_hotspotClientsBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_bluetoothEnabledBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_batteryLevelBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_unreadCountBridge;

	static lv_obj_t* s_overlayLabel;
	static lv_obj_t* s_statusBarInstance;
};

} // namespace UI::Modules
