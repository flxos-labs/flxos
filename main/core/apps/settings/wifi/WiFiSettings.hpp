#pragma once
#include "core/apps/settings/SettingsPageBase.hpp"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "lvgl.h"
#include <cstring>
#include <string>
#include <vector>

namespace System::Apps::Settings {

class WiFiSettings : public SettingsPageBase {
public:

	using SettingsPageBase::SettingsPageBase;

	void showConfig();
	void hideConfig();
	void refreshScan();
	void updateStatus();
	void showConnectScreen(const char* ssid);

protected:

	void createUI() override;
	void onShow() override;
	void onHide() override;
	void onDestroy() override;

private:

	static const char* getSignalIcon(int8_t rssi);

	lv_obj_t* m_connectContainer = nullptr;
	lv_obj_t* m_configContainer = nullptr;
	lv_obj_t* m_wifiSwitch = nullptr;
	lv_obj_t* m_statusLabel = nullptr;
	lv_obj_t* m_statusPrefixLabel = nullptr;
	lv_obj_t* m_passwordTa = nullptr;
	lv_obj_t* m_saveSwitch = nullptr;
	std::string m_connectSsid;
	bool m_isScanning = false;
	bool m_pendingAutoScan = false;
	bool m_destroying = false;
	lv_observer_t* m_statusObserver = nullptr;
	lv_observer_t* m_scanIntervalObserver = nullptr;
	esp_timer_handle_t m_scanTimer = nullptr;
	std::vector<wifi_ap_record_t> m_scanResults;
};

} // namespace System::Apps::Settings
