#pragma once
#include "esp_wifi.h"
#include "lvgl.h"
#include <cstring>
#include <functional>
#include <string>
#include <vector>

namespace System::Apps::Settings {

class WiFiSettings {
public:

	WiFiSettings(lv_obj_t* parent, std::function<void()> onBack);

	void show();
	void showConfig();
	void hideConfig();
	void refreshScan();
	void updateStatus();
	void showConnectScreen(const char* ssid);
	void hide();
	void destroy();

private:

	static const char* getSignalIcon(int8_t rssi);

	lv_obj_t* m_parent;
	lv_obj_t* m_container = nullptr;
	lv_obj_t* m_connectContainer = nullptr;
	lv_obj_t* m_configContainer = nullptr;
	lv_obj_t* m_list = nullptr;
	lv_obj_t* m_wifiSwitch = nullptr;
	lv_obj_t* m_statusLabel = nullptr;
	lv_obj_t* m_statusPrefixLabel = nullptr;
	lv_obj_t* m_passwordTa = nullptr;
	std::string m_connectSsid {};
	lv_timer_t* m_timer = nullptr;
	bool m_isScanning = false;
	std::function<void()> m_onBack {};
	std::vector<wifi_ap_record_t> m_scanResults {};
};

} // namespace System::Apps::Settings
