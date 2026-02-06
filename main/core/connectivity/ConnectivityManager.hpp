#pragma once

#include "core/common/Observable.hpp"
#include "esp_err.h"
#include "esp_event.h"
#include "hotspot/HotspotManager.hpp"
#include "wifi/WiFiManager.hpp"
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#if !CONFIG_FLXOS_HEADLESS_MODE
#include "core/ui/LvglObserverBridge.hpp"
#endif

namespace System {

class ConnectivityManager {
public:

	static ConnectivityManager& getInstance();

	esp_err_t init();

#if !CONFIG_FLXOS_HEADLESS_MODE
	void initGuiBridges(); // Initialize LVGL bridges for GUI mode
#endif

	// Mode management
	esp_err_t setWifiMode(wifi_mode_t mode, bool auto_start = true);
	std::recursive_mutex& getWifiMutex() { return m_wifi_mutex; }

	// WiFi Station
	esp_err_t connectWiFi(const char* ssid, const char* password, bool remember = true);
	esp_err_t disconnectWiFi();
	bool isWiFiConnected();
	esp_err_t scanWiFi(WiFiManager::ScanCallback callback);
	esp_err_t setWiFiEnabled(bool enabled);
	bool isWiFiEnabled();
	void saveWiFiCredentials(const char* ssid, const char* password);
	void clearSavedWiFiCredentials();
	bool hasSavedWiFiCredentials() const;

	// WiFi Hotspot (SoftAP)
	esp_err_t startHotspot(const char* ssid, const char* password, int channel = 1, int max_connections = 4, bool hidden = false, wifi_auth_mode_t auth_mode = WIFI_AUTH_WPA2_PSK, int8_t max_tx_power = 80);
	esp_err_t stopHotspot();
	static void startHotspotUsageTimer() {
		HotspotManager::getInstance().startUsageTimer();
	}
	bool isHotspotEnabled();
	std::vector<HotspotManager::ClientInfo> getHotspotClientsList() const;

	static esp_err_t setHotspotNatEnabled(bool enabled) {
		return HotspotManager::getInstance().setNatEnabled(enabled);
	}
	static bool isHotspotNatEnabled() {
		return HotspotManager::getInstance().isNatEnabled();
	}

	// Bluetooth
	esp_err_t enableBluetooth(bool enable);
	bool isBluetoothEnabled();

	// Observable subjects (headless-compatible getters)
	Observable<int32_t>& getWiFiEnabledObservable() { return m_wifi_enabled_subject; }
	Observable<int32_t>& getWiFiStatusObservable() { return m_wifi_status_subject; }
	Observable<int32_t>& getWiFiConnectedObservable() { return m_wifi_connected_subject; }
	StringObservable& getWiFiSsidObservable() { return m_wifi_ssid_subject; }
	StringObservable& getWiFiIpObservable() { return m_wifi_ip_subject; }
	Observable<int32_t>& getHotspotEnabledObservable() { return m_hotspot_enabled_subject; }
	Observable<int32_t>& getHotspotClientsObservable() { return m_hotspot_clients_subject; }
	Observable<int32_t>& getHotspotUsageSentSubject() { return m_hotspot_usage_sent_subject; }
	Observable<int32_t>& getHotspotUsageReceivedSubject() { return m_hotspot_usage_received_subject; }
	Observable<int32_t>& getHotspotUploadSpeedSubject() { return m_hotspot_upload_speed_subject; }
	Observable<int32_t>& getHotspotDownloadSpeedSubject() { return m_hotspot_download_speed_subject; }
	Observable<int32_t>& getHotspotUptimeSubject() { return m_hotspot_uptime_subject; }
	Observable<int32_t>& getBluetoothEnabledObservable() { return m_bluetooth_enabled_subject; }

	// Config Settings (Persisted)
	StringObservable& getHotspotSsidObservable() { return m_hotspot_ssid_subject; }
	StringObservable& getHotspotPasswordObservable() { return m_hotspot_password_subject; }
	Observable<int32_t>& getHotspotChannelObservable() { return m_hotspot_channel_subject; }
	Observable<int32_t>& getHotspotMaxConnObservable() { return m_hotspot_max_conn_subject; }
	Observable<int32_t>& getHotspotHiddenObservable() { return m_hotspot_hidden_subject; }
	Observable<int32_t>& getHotspotAuthObservable() { return m_hotspot_auth_subject; }
	Observable<int32_t>& getWiFiAutostartObservable() { return m_wifi_autostart_subject; }
	StringObservable& getSavedWiFiSsidObservable() { return m_saved_wifi_ssid_subject; }
	StringObservable& getSavedWiFiPasswordObservable() { return m_saved_wifi_password_subject; }

#if !CONFIG_FLXOS_HEADLESS_MODE
	// GUI-only: LVGL subject accessors (for use with lv_subject_add_observer)
	lv_subject_t& getWiFiEnabledSubject() { return *m_wifi_enabled_bridge->getSubject(); }
	lv_subject_t& getWiFiStatusSubject() { return *m_wifi_status_bridge->getSubject(); }
	lv_subject_t& getWiFiConnectedSubject() { return *m_wifi_connected_bridge->getSubject(); }
	lv_subject_t& getWiFiSsidSubject() { return *m_wifi_ssid_bridge->getSubject(); }
	lv_subject_t& getWiFiIpSubject() { return *m_wifi_ip_bridge->getSubject(); }
	lv_subject_t& getHotspotEnabledSubject() { return *m_hotspot_enabled_bridge->getSubject(); }
	lv_subject_t& getHotspotClientsSubject() { return *m_hotspot_clients_bridge->getSubject(); }
	lv_subject_t& getHotspotUsageSentLvglSubject() { return *m_hotspot_usage_sent_bridge->getSubject(); }
	lv_subject_t& getHotspotUsageReceivedLvglSubject() { return *m_hotspot_usage_received_bridge->getSubject(); }
	lv_subject_t& getHotspotUploadSpeedLvglSubject() { return *m_hotspot_upload_speed_bridge->getSubject(); }
	lv_subject_t& getHotspotDownloadSpeedLvglSubject() { return *m_hotspot_download_speed_bridge->getSubject(); }
	lv_subject_t& getHotspotUptimeLvglSubject() { return *m_hotspot_uptime_bridge->getSubject(); }
	lv_subject_t& getBluetoothEnabledSubject() { return *m_bluetooth_enabled_bridge->getSubject(); }

	lv_subject_t& getHotspotSsidSubject() { return *m_hotspot_ssid_bridge->getSubject(); }
	lv_subject_t& getHotspotPasswordSubject() { return *m_hotspot_password_bridge->getSubject(); }
	lv_subject_t& getHotspotChannelSubject() { return *m_hotspot_channel_bridge->getSubject(); }
	lv_subject_t& getHotspotMaxConnSubject() { return *m_hotspot_max_conn_bridge->getSubject(); }
	lv_subject_t& getHotspotHiddenSubject() { return *m_hotspot_hidden_bridge->getSubject(); }
	lv_subject_t& getHotspotAuthSubject() { return *m_hotspot_auth_bridge->getSubject(); }
	lv_subject_t& getWiFiAutostartSubject() { return *m_wifi_autostart_bridge->getSubject(); }
#endif

private:

	ConnectivityManager() = default;
	~ConnectivityManager() = default;
	ConnectivityManager(const ConnectivityManager&) = delete;
	ConnectivityManager& operator=(const ConnectivityManager&) = delete;

	Observable<int32_t> m_wifi_enabled_subject {0};
	Observable<int32_t> m_wifi_status_subject {0};
	Observable<int32_t> m_wifi_connected_subject {0};
	StringObservable m_wifi_ssid_subject {"Disconnected"};
	StringObservable m_wifi_ip_subject {"0.0.0.0"};

	Observable<int32_t> m_hotspot_enabled_subject {0};
	Observable<int32_t> m_hotspot_clients_subject {0};
	Observable<int32_t> m_hotspot_usage_sent_subject {0};
	Observable<int32_t> m_hotspot_usage_received_subject {0};
	Observable<int32_t> m_hotspot_upload_speed_subject {0};
	Observable<int32_t> m_hotspot_download_speed_subject {0};
	Observable<int32_t> m_hotspot_uptime_subject {0};
	Observable<int32_t> m_bluetooth_enabled_subject {0};

	// Config Settings (Persisted)
	StringObservable m_hotspot_ssid_subject {"ESP32-Hotspot"};
	StringObservable m_hotspot_password_subject {"12345678"};
	Observable<int32_t> m_hotspot_channel_subject {1};
	Observable<int32_t> m_hotspot_max_conn_subject {4};
	Observable<int32_t> m_hotspot_hidden_subject {0};
	Observable<int32_t> m_hotspot_auth_subject {1};
	Observable<int32_t> m_wifi_autostart_subject {0};
	StringObservable m_saved_wifi_ssid_subject {""};
	StringObservable m_saved_wifi_password_subject {""};

#if !CONFIG_FLXOS_HEADLESS_MODE
	// LVGL bridges (initialized in initGuiBridges)
	std::unique_ptr<LvglObserverBridge<int32_t>> m_wifi_enabled_bridge {};
	std::unique_ptr<LvglObserverBridge<int32_t>> m_wifi_status_bridge {};
	std::unique_ptr<LvglObserverBridge<int32_t>> m_wifi_connected_bridge {};
	std::unique_ptr<LvglStringObserverBridge> m_wifi_ssid_bridge {};
	std::unique_ptr<LvglStringObserverBridge> m_wifi_ip_bridge {};
	std::unique_ptr<LvglObserverBridge<int32_t>> m_hotspot_enabled_bridge {};
	std::unique_ptr<LvglObserverBridge<int32_t>> m_hotspot_clients_bridge {};
	std::unique_ptr<LvglObserverBridge<int32_t>> m_hotspot_usage_sent_bridge {};
	std::unique_ptr<LvglObserverBridge<int32_t>> m_hotspot_usage_received_bridge {};
	std::unique_ptr<LvglObserverBridge<int32_t>> m_hotspot_upload_speed_bridge {};
	std::unique_ptr<LvglObserverBridge<int32_t>> m_hotspot_download_speed_bridge {};
	std::unique_ptr<LvglObserverBridge<int32_t>> m_hotspot_uptime_bridge {};
	std::unique_ptr<LvglObserverBridge<int32_t>> m_bluetooth_enabled_bridge {};

	std::unique_ptr<LvglStringObserverBridge> m_hotspot_ssid_bridge {};
	std::unique_ptr<LvglStringObserverBridge> m_hotspot_password_bridge {};
	std::unique_ptr<LvglObserverBridge<int32_t>> m_hotspot_channel_bridge {};
	std::unique_ptr<LvglObserverBridge<int32_t>> m_hotspot_max_conn_bridge {};
	std::unique_ptr<LvglObserverBridge<int32_t>> m_hotspot_hidden_bridge {};
	std::unique_ptr<LvglObserverBridge<int32_t>> m_hotspot_auth_bridge {};
	std::unique_ptr<LvglObserverBridge<int32_t>> m_wifi_autostart_bridge {};
#endif

	bool m_is_init = false;
	std::recursive_mutex m_wifi_mutex {};
};

} // namespace System
