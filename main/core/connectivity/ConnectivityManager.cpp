#include "ConnectivityManager.hpp"
#include "bluetooth/BluetoothManager.hpp"
#include "core/common/Logger.hpp"
#include "core/system/settings/SettingsManager.hpp"
#include "core/tasks/gui/GuiTask.hpp"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "esp_wifi_types_generic.h"
#include "hotspot/HotspotManager.hpp"
#include "wifi/WiFiManager.hpp"
#include <cstdint>
#include <string_view>

static constexpr std::string_view TAG = "Connectivity";

namespace System {
ConnectivityManager& ConnectivityManager::getInstance() {
	static ConnectivityManager instance;
	return instance;
}

esp_err_t ConnectivityManager::init() {
	if (m_is_init) {
		return ESP_OK;
	}
	Log::info(TAG, "Initializing networking stack...");
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_sta();
	esp_netif_create_default_wifi_ap();
	wifi_init_config_t const cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	// Initialize sub-managers with observable references
	WiFiManager::getInstance().init(&m_wifi_connected_subject, &m_wifi_ssid_subject, &m_wifi_ip_subject, &m_wifi_status_subject);
	HotspotManager::getInstance().init(&m_hotspot_enabled_subject, &m_hotspot_clients_subject);
	BluetoothManager::getInstance().init(&m_bluetooth_enabled_subject);

	// Register config settings for persistence
	SettingsManager::getInstance().registerSetting("hs_ssid", m_hotspot_ssid_subject);
	SettingsManager::getInstance().registerSetting("hs_pass", m_hotspot_password_subject);
	SettingsManager::getInstance().registerSetting("hs_chan", m_hotspot_channel_subject);
	SettingsManager::getInstance().registerSetting("hs_max", m_hotspot_max_conn_subject);
	SettingsManager::getInstance().registerSetting("hs_hide", m_hotspot_hidden_subject);
	SettingsManager::getInstance().registerSetting("hs_auth", m_hotspot_auth_subject);
	SettingsManager::getInstance().registerSetting("wifi_autostart", m_wifi_autostart_subject);
	SettingsManager::getInstance().registerSetting("wifi_scan_int", m_wifi_scan_interval_subject);
	SettingsManager::getInstance().registerSetting("wifi_ssid", m_saved_wifi_ssid_subject);
	SettingsManager::getInstance().registerSetting("wifi_pass", m_saved_wifi_password_subject);

	ESP_ERROR_CHECK(setWifiMode(WIFI_MODE_NULL));
	m_is_init = true;
	Log::info(TAG, "ConnectivityManager initialized");

	// Auto-start WiFi and connect to saved network if enabled
	if (m_wifi_autostart_subject.get() != 0) {
		Log::info(TAG, "Auto-starting WiFi...");
		setWiFiEnabled(true);

		// Try to connect to saved network
		if (hasSavedWiFiCredentials()) {
			const char* saved_ssid = m_saved_wifi_ssid_subject.get();
			const char* saved_pass = m_saved_wifi_password_subject.get();
			Log::info(TAG, "Auto-connecting to saved network: %s", saved_ssid);
			WiFiManager::getInstance().connect(saved_ssid, saved_pass);
		}
	}

	return ESP_OK;
}

#if !CONFIG_FLXOS_HEADLESS_MODE
void ConnectivityManager::initGuiBridges() {
	Log::info(TAG, "Initializing LVGL bridges for connectivity...");
	m_wifi_enabled_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_wifi_enabled_subject);
	m_wifi_status_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_wifi_status_subject);
	m_wifi_connected_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_wifi_connected_subject);
	m_wifi_ssid_bridge = std::make_unique<LvglStringObserverBridge>(m_wifi_ssid_subject);
	m_wifi_ip_bridge = std::make_unique<LvglStringObserverBridge>(m_wifi_ip_subject);
	m_hotspot_enabled_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_hotspot_enabled_subject);
	m_hotspot_clients_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_hotspot_clients_subject);
	m_hotspot_usage_sent_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_hotspot_usage_sent_subject);
	m_hotspot_usage_received_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_hotspot_usage_received_subject);
	m_hotspot_download_speed_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_hotspot_download_speed_subject);
	m_hotspot_upload_speed_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_hotspot_upload_speed_subject);
	m_hotspot_uptime_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_hotspot_uptime_subject);
	m_bluetooth_enabled_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_bluetooth_enabled_subject);

	m_hotspot_ssid_bridge = std::make_unique<LvglStringObserverBridge>(m_hotspot_ssid_subject);
	m_hotspot_password_bridge = std::make_unique<LvglStringObserverBridge>(m_hotspot_password_subject);
	m_hotspot_channel_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_hotspot_channel_subject);
	m_hotspot_max_conn_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_hotspot_max_conn_subject);
	m_hotspot_hidden_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_hotspot_hidden_subject);
	m_hotspot_auth_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_hotspot_auth_subject);
	m_wifi_autostart_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_wifi_autostart_subject);
	m_wifi_scan_interval_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_wifi_scan_interval_subject);
}
#endif

esp_err_t ConnectivityManager::setWifiMode(wifi_mode_t mode, bool auto_start) {
	std::lock_guard<std::recursive_mutex> lock(m_wifi_mutex);

	wifi_mode_t current_mode;
	esp_err_t err = esp_wifi_get_mode(&current_mode);
	if (err != ESP_OK && err != ESP_ERR_WIFI_NOT_INIT) {
		return err;
	}

	if (err != ESP_ERR_WIFI_NOT_INIT && current_mode == mode) {
		if (auto_start) {
			return esp_wifi_start(); // Ensure it's started
		}
		return ESP_OK;
	}

	Log::info(TAG, "Setting WiFi mode: %d", (int)mode);
	err = esp_wifi_set_mode(mode);
	if (err != ESP_OK) {
		Log::error("Connectivity", "Failed to set WiFi mode: %d (0x%x)", (int)mode, err);
		return err;
	}

	if (auto_start) {
		return esp_wifi_start();
	}
	return ESP_OK;
}

esp_err_t ConnectivityManager::connectWiFi(const char* ssid, const char* password, bool remember) {
	esp_err_t err = WiFiManager::getInstance().connect(ssid, password);
	if (err == ESP_OK && remember) {
		saveWiFiCredentials(ssid, password);
	}
	return err;
}
esp_err_t ConnectivityManager::disconnectWiFi() {
	return WiFiManager::getInstance().disconnect();
}
bool ConnectivityManager::isWiFiConnected() {
	return WiFiManager::getInstance().isConnected();
}
esp_err_t ConnectivityManager::scanWiFi(WiFiManager::ScanCallback callback) {
	return WiFiManager::getInstance().scan(callback);
}
esp_err_t ConnectivityManager::setWiFiEnabled(bool enabled) {
	Log::info(TAG, "WiFi enabled set to: %s", enabled ? "TRUE" : "FALSE");
	esp_err_t const err = WiFiManager::getInstance().setEnabled(enabled);
	if (err == ESP_OK) {
		GuiTask::lock();
		m_wifi_enabled_subject.set(enabled ? 1 : 0);
		GuiTask::unlock();
	}
	return err;
}
bool ConnectivityManager::isWiFiEnabled() {
	return WiFiManager::getInstance().isEnabled();
}
esp_err_t ConnectivityManager::startHotspot(const char* s, const char* p, int c, int m, bool h, wifi_auth_mode_t auth, int8_t tx) {
	Log::info(TAG, "Starting Hotspot (SSID: %s)", s);
	return HotspotManager::getInstance().start(s, p, c, m, h, auth, tx);
}
esp_err_t ConnectivityManager::stopHotspot() {
	Log::info(TAG, "Stopping Hotspot");
	return HotspotManager::getInstance().stop();
}
bool ConnectivityManager::isHotspotEnabled() {
	return HotspotManager::getInstance().isEnabled();
}
std::vector<HotspotManager::ClientInfo>
ConnectivityManager::getHotspotClientsList() const {
	return HotspotManager::getInstance().getConnectedClients();
}
esp_err_t ConnectivityManager::enableBluetooth(bool e) {
	return BluetoothManager::getInstance().enable(e);
}
bool ConnectivityManager::isBluetoothEnabled() {
	return BluetoothManager::getInstance().isEnabled();
}

void ConnectivityManager::saveWiFiCredentials(const char* ssid, const char* password) {
	m_saved_wifi_ssid_subject.set(ssid ? ssid : "");
	m_saved_wifi_password_subject.set(password ? password : "");
	Log::info(TAG, "WiFi credentials saved for: %s", ssid);
}

void ConnectivityManager::clearSavedWiFiCredentials() {
	m_saved_wifi_ssid_subject.set("");
	m_saved_wifi_password_subject.set("");
	Log::info(TAG, "Saved WiFi credentials cleared");
}

bool ConnectivityManager::hasSavedWiFiCredentials() const {
	const char* ssid = m_saved_wifi_ssid_subject.get();
	return ssid != nullptr && ssid[0] != '\0';
}

} // namespace System
