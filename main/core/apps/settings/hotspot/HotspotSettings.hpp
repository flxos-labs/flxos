#pragma once
#include "core/apps/settings/SettingsPageBase.hpp"
#include "core/ui/LvglObserverBridge.hpp"
#include "lvgl.h"
#include <cstring>
#include <functional>
#include <memory>
#include <string>

namespace System::Apps::Settings {

class HotspotSettings : public SettingsPageBase {
public:

	using SettingsPageBase::SettingsPageBase;

protected:

	void createUI() override;
	void onShow() override;
	void onDestroy() override;

private:

	void createMainPage();
	void createConfigPage();
	void showMainPage();
	void showConfigPage();
	void saveAndApply();
	void applyHotspotSettings();

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
	bool m_ignore_events = false;

	std::unique_ptr<System::LvglObserverBridge<int32_t>> m_hotspotEnabledBridge;
	std::unique_ptr<System::LvglObserverBridge<int32_t>> m_clientCountBridge;
	std::unique_ptr<System::LvglObserverBridge<int32_t>> m_usageSentBridge;
	std::unique_ptr<System::LvglObserverBridge<int32_t>> m_usageReceivedBridge;
	std::unique_ptr<System::LvglObserverBridge<int32_t>> m_uploadSpeedBridge;
	std::unique_ptr<System::LvglObserverBridge<int32_t>> m_downloadSpeedBridge;
	std::unique_ptr<System::LvglObserverBridge<int32_t>> m_uptimeBridge;

	std::unique_ptr<System::LvglStringObserverBridge> m_ssidBridge;
	std::unique_ptr<System::LvglStringObserverBridge> m_passwordBridge;
	std::unique_ptr<System::LvglObserverBridge<int32_t>> m_channelBridge;
	std::unique_ptr<System::LvglObserverBridge<int32_t>> m_maxConnBridge;
	std::unique_ptr<System::LvglObserverBridge<int32_t>> m_hiddenBridge;
	std::unique_ptr<System::LvglObserverBridge<int32_t>> m_authBridge;
	std::unique_ptr<System::LvglObserverBridge<int32_t>> m_autoShutdownBridge;
};

} // namespace System::Apps::Settings
