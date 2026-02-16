#pragma once

#include "esp_err.h"
#include "esp_event.h"
#include "flx/connectivity/hotspot/HotspotManager.hpp"
#include "flx/connectivity/wifi/WiFiManager.hpp"
#include <flx/core/Observable.hpp>
#include <flx/core/Singleton.hpp>
#include <flx/services/IService.hpp>
#include <flx/services/ServiceManifest.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace flx::connectivity {

class ConnectivityManager : public flx::Singleton<ConnectivityManager>, public flx::services::IService {
	friend class flx::Singleton<ConnectivityManager>;

public:

	// ──── IService manifest ────
	static const flx::services::ServiceManifest serviceManifest;
	const flx::services::ServiceManifest& getManifest() const override { return serviceManifest; }

	// ──── IService lifecycle ────
	bool onStart() override;
	void onStop() override;

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
	std::string getSavedWiFiSsid() const { return m_saved_wifi_ssid_subject.get(); }
	std::string getSavedWiFiPassword() const { return m_saved_wifi_password_subject.get(); }

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

	// flx::Observable subjects (headless-compatible getters)
	flx::Observable<int32_t>& getWiFiEnabledObservable() { return m_wifi_enabled_subject; }
	flx::Observable<int32_t>& getWiFiStatusObservable() { return m_wifi_status_subject; }
	flx::Observable<int32_t>& getWiFiConnectedObservable() { return m_wifi_connected_subject; }
	flx::StringObservable& getWiFiSsidObservable() { return m_wifi_ssid_subject; }
	flx::StringObservable& getWiFiIpObservable() { return m_wifi_ip_subject; }
	flx::Observable<int32_t>& getHotspotEnabledObservable() { return m_hotspot_enabled_subject; }
	flx::Observable<int32_t>& getHotspotClientsObservable() { return m_hotspot_clients_subject; }
	flx::Observable<int32_t>& getHotspotUsageSentSubject() { return m_hotspot_usage_sent_subject; }
	flx::Observable<int32_t>& getHotspotUsageReceivedSubject() { return m_hotspot_usage_received_subject; }
	flx::Observable<int32_t>& getHotspotUploadSpeedSubject() { return m_hotspot_upload_speed_subject; }
	flx::Observable<int32_t>& getHotspotDownloadSpeedSubject() { return m_hotspot_download_speed_subject; }
	flx::Observable<int32_t>& getHotspotUptimeSubject() { return m_hotspot_uptime_subject; }
	flx::Observable<int32_t>& getBluetoothEnabledObservable() { return m_bluetooth_enabled_subject; }

	// Config Settings (Persisted)
	flx::StringObservable& getHotspotSsidObservable() { return m_hotspot_ssid_subject; }
	flx::StringObservable& getHotspotPasswordObservable() { return m_hotspot_password_subject; }
	flx::Observable<int32_t>& getHotspotChannelObservable() { return m_hotspot_channel_subject; }
	flx::Observable<int32_t>& getHotspotMaxConnObservable() { return m_hotspot_max_conn_subject; }
	flx::Observable<int32_t>& getHotspotHiddenObservable() { return m_hotspot_hidden_subject; }
	flx::Observable<int32_t>& getHotspotAuthObservable() { return m_hotspot_auth_subject; }
	flx::Observable<int32_t>& getWiFiAutostartObservable() { return m_wifi_autostart_subject; }
	flx::Observable<int32_t>& getWiFiScanIntervalObservable() { return m_wifi_scan_interval_subject; }
	flx::StringObservable& getSavedWiFiSsidObservable() { return m_saved_wifi_ssid_subject; }
	flx::StringObservable& getSavedWiFiPasswordObservable() { return m_saved_wifi_password_subject; }

private:

	ConnectivityManager() = default;
	~ConnectivityManager() = default;

	flx::Observable<int32_t> m_wifi_enabled_subject {0};
	flx::Observable<int32_t> m_wifi_status_subject {0};
	flx::Observable<int32_t> m_wifi_connected_subject {0};
	flx::StringObservable m_wifi_ssid_subject {"Disconnected"};
	flx::StringObservable m_wifi_ip_subject {"0.0.0.0"};

	flx::Observable<int32_t> m_hotspot_enabled_subject {0};
	flx::Observable<int32_t> m_hotspot_clients_subject {0};
	flx::Observable<int32_t> m_hotspot_usage_sent_subject {0};
	flx::Observable<int32_t> m_hotspot_usage_received_subject {0};
	flx::Observable<int32_t> m_hotspot_upload_speed_subject {0};
	flx::Observable<int32_t> m_hotspot_download_speed_subject {0};
	flx::Observable<int32_t> m_hotspot_uptime_subject {0};
	flx::Observable<int32_t> m_bluetooth_enabled_subject {0};

	// Config Settings (Persisted)
	flx::StringObservable m_hotspot_ssid_subject {"ESP32-Hotspot"};
	flx::StringObservable m_hotspot_password_subject {"12345678"};
	flx::Observable<int32_t> m_hotspot_channel_subject {1};
	flx::Observable<int32_t> m_hotspot_max_conn_subject {4};
	flx::Observable<int32_t> m_hotspot_hidden_subject {0};
	flx::Observable<int32_t> m_hotspot_auth_subject {1};
	flx::Observable<int32_t> m_wifi_autostart_subject {0};
	flx::Observable<int32_t> m_wifi_scan_interval_subject {0};
	flx::StringObservable m_saved_wifi_ssid_subject {""};
	flx::StringObservable m_saved_wifi_password_subject {""};

	std::recursive_mutex m_wifi_mutex {};
};

} // namespace flx::connectivity
