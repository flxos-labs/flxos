#pragma once

#include "esp_err.h"
#include "esp_event.h"
#include "hotspot/HotspotManager.hpp"
#include "lvgl.h"
#include "wifi/WiFiManager.hpp"
#include <mutex>
#include <string>
#include <vector>

namespace System {

class ConnectivityManager {
public:

	static ConnectivityManager& getInstance();

	esp_err_t init();

	// Mode management
	esp_err_t setWifiMode(wifi_mode_t mode);
	std::recursive_mutex& getWifiMutex() { return m_wifi_mutex; }

	// WiFi Station
	esp_err_t connectWiFi(const char* ssid, const char* password);
	esp_err_t disconnectWiFi();
	bool isWiFiConnected() const;
	esp_err_t scanWiFi(WiFiManager::ScanCallback callback);
	esp_err_t setWiFiEnabled(bool enabled);
	bool isWiFiEnabled() const;

	// WiFi Hotspot (SoftAP)
	esp_err_t startHotspot(const char* ssid, const char* password, int channel = 1, int max_connections = 4, bool hidden = false, wifi_auth_mode_t auth_mode = WIFI_AUTH_WPA2_PSK, int8_t max_tx_power = 80);
	esp_err_t stopHotspot();
	void startHotspotUsageTimer() {
		HotspotManager::getInstance().startUsageTimer();
	}
	bool isHotspotEnabled() const;
	std::vector<HotspotManager::ClientInfo> getHotspotClientsList() const;

	esp_err_t setHotspotNatEnabled(bool enabled) {
		return HotspotManager::getInstance().setNatEnabled(enabled);
	}
	bool isHotspotNatEnabled() const {
		return HotspotManager::getInstance().isNatEnabled();
	}

	// Bluetooth
	esp_err_t enableBluetooth(bool enable);
	bool isBluetoothEnabled() const;

	// Subjects for UI binding
	lv_subject_t& getWiFiEnabledSubject() { return m_wifi_enabled_subject; }
	lv_subject_t& getWiFiStatusSubject() { return m_wifi_status_subject; }
	lv_subject_t& getWiFiConnectedSubject() { return m_wifi_connected_subject; }
	lv_subject_t& getWiFiSsidSubject() { return m_wifi_ssid_subject; }
	lv_subject_t& getWiFiIpSubject() { return m_wifi_ip_subject; }
	lv_subject_t& getHotspotEnabledSubject() { return m_hotspot_enabled_subject; }
	lv_subject_t& getHotspotClientsSubject() { return m_hotspot_clients_subject; }
	lv_subject_t& getHotspotUsageSentSubject() {
		return m_hotspot_usage_sent_subject;
	}
	lv_subject_t& getHotspotUsageReceivedSubject() {
		return m_hotspot_usage_received_subject;
	}
	lv_subject_t& getHotspotUploadSpeedSubject() {
		return m_hotspot_upload_speed_subject;
	}
	lv_subject_t& getHotspotDownloadSpeedSubject() {
		return m_hotspot_download_speed_subject;
	}
	lv_subject_t& getHotspotUptimeSubject() { return m_hotspot_uptime_subject; }
	lv_subject_t& getBluetoothEnabledSubject() {
		return m_bluetooth_enabled_subject;
	}

private:

	ConnectivityManager() = default;
	~ConnectivityManager() = default;
	ConnectivityManager(const ConnectivityManager&) = delete;
	ConnectivityManager& operator=(const ConnectivityManager&) = delete;

	lv_subject_t m_wifi_enabled_subject;
	lv_subject_t m_wifi_status_subject;
	lv_subject_t m_wifi_connected_subject;
	lv_subject_t m_wifi_ssid_subject;
	char m_wifi_ssid_buffer[33];
	char m_wifi_ssid_prev_buffer[33];

	lv_subject_t m_wifi_ip_subject;
	char m_wifi_ip_buffer[16];
	char m_wifi_ip_prev_buffer[16];

	lv_subject_t m_hotspot_enabled_subject;
	lv_subject_t m_hotspot_clients_subject;
	lv_subject_t m_hotspot_usage_sent_subject;
	lv_subject_t m_hotspot_usage_received_subject;
	lv_subject_t m_hotspot_upload_speed_subject;
	lv_subject_t m_hotspot_download_speed_subject;
	lv_subject_t m_hotspot_uptime_subject;
	lv_subject_t m_bluetooth_enabled_subject;

	bool m_is_init = false;
	std::recursive_mutex m_wifi_mutex;
};

} // namespace System
