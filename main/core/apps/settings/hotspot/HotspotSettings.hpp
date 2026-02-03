#pragma once
#include "lvgl.h"
#include <cstring>
#include <functional>
#include <string>

namespace System {
namespace Apps {
namespace Settings {

class HotspotSettings {
public:

	HotspotSettings(lv_obj_t* parent, std::function<void()> onBack);

	void show();
	void hide();
	void destroy();

private:

	void createMainPage();
	void createConfigPage();
	void showMainPage();
	void showConfigPage();
	void saveAndApply();
	void applyHotspotSettings();

	lv_obj_t* m_parent;
	lv_obj_t* m_container = nullptr;
	lv_obj_t* m_mainPage = nullptr;
	lv_obj_t* m_configPage = nullptr;
	lv_obj_t* m_hotspotSwitch = nullptr;
	lv_obj_t* m_ssidTa = nullptr;
	lv_obj_t* m_passwordTa = nullptr;
	lv_obj_t* m_channelDropdown = nullptr;
	lv_obj_t* m_maxConnSlider = nullptr;
	lv_obj_t* m_hiddenSwitch = nullptr;
	lv_obj_t* m_natSwitch = nullptr;
	lv_obj_t* m_securityDropdown = nullptr;
	lv_obj_t* m_txPowerSlider = nullptr;
	lv_obj_t* m_autoShutdownSwitch = nullptr;
	lv_obj_t* m_clientsCont = nullptr;
	lv_obj_t* m_configTitle = nullptr;
	lv_timer_t* m_refreshTimer = nullptr;
	std::function<void()> m_onBack;
};

} // namespace Settings
} // namespace Apps
} // namespace System
