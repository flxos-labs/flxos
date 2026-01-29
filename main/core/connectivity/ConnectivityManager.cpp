#include "ConnectivityManager.hpp"
#include "bluetooth/BluetoothManager.hpp"
#include "core/tasks/gui/GuiTask.hpp"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "hotspot/HotspotManager.hpp"
#include "wifi/WiFiManager.hpp"

static const char* TAG = "ConnectivityManager";

namespace System {
ConnectivityManager& ConnectivityManager::getInstance() {
	static ConnectivityManager instance;
	return instance;
}

esp_err_t ConnectivityManager::init() {
	if (m_is_init)
		return ESP_OK;
	ESP_LOGI(TAG, "Initializing Connectivity...");
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_sta();
	esp_netif_create_default_wifi_ap();
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	GuiTask::lock();
	lv_subject_init_int(&m_wifi_enabled_subject, 0);
	lv_subject_init_int(&m_wifi_status_subject, 0);
	lv_subject_init_int(&m_wifi_connected_subject, 0);
	lv_subject_init_string(&m_wifi_ssid_subject, m_wifi_ssid_buffer, m_wifi_ssid_prev_buffer, sizeof(m_wifi_ssid_buffer), "Disconnected");
	lv_subject_init_string(&m_wifi_ip_subject, m_wifi_ip_buffer, m_wifi_ip_prev_buffer, sizeof(m_wifi_ip_buffer), "0.0.0.0");
	lv_subject_init_int(&m_hotspot_enabled_subject, 0);
	lv_subject_init_int(&m_hotspot_clients_subject, 0);
	lv_subject_init_int(&m_hotspot_usage_sent_subject, 0);
	lv_subject_init_int(&m_hotspot_usage_received_subject, 0);
	lv_subject_init_int(&m_hotspot_upload_speed_subject, 0);
	lv_subject_init_int(&m_hotspot_download_speed_subject, 0);
	lv_subject_init_int(&m_hotspot_uptime_subject, 0);
	lv_subject_init_int(&m_bluetooth_enabled_subject, 0);
	GuiTask::unlock();

	WiFiManager::getInstance().init(&m_wifi_connected_subject, &m_wifi_ssid_subject, &m_wifi_ip_subject, &m_wifi_status_subject);
	HotspotManager::getInstance().init(&m_hotspot_enabled_subject, &m_hotspot_clients_subject);
	BluetoothManager::getInstance().init(&m_bluetooth_enabled_subject);

	ESP_ERROR_CHECK(setWifiMode(WIFI_MODE_NULL));
	m_is_init = true;
	return ESP_OK;
}

esp_err_t ConnectivityManager::setWifiMode(wifi_mode_t mode) {
	std::lock_guard<std::recursive_mutex> lock(m_wifi_mutex);

	wifi_mode_t current_mode;
	esp_err_t err = esp_wifi_get_mode(&current_mode);
	if (err != ESP_OK && err != ESP_ERR_WIFI_NOT_INIT) {
		return err;
	}

	if (err != ESP_ERR_WIFI_NOT_INIT && current_mode == mode) {
		return esp_wifi_start(); // Ensure it's started even if mode is same
	}

	err = esp_wifi_set_mode(mode);
	if (err != ESP_OK)
		return err;

	return esp_wifi_start();
}

esp_err_t ConnectivityManager::connectWiFi(const char* s, const char* p) {
	return WiFiManager::getInstance().connect(s, p);
}
esp_err_t ConnectivityManager::disconnectWiFi() {
	return WiFiManager::getInstance().disconnect();
}
bool ConnectivityManager::isWiFiConnected() const {
	return WiFiManager::getInstance().isConnected();
}
esp_err_t ConnectivityManager::scanWiFi(WiFiManager::ScanCallback callback) {
	return WiFiManager::getInstance().scan(callback);
}
esp_err_t ConnectivityManager::setWiFiEnabled(bool enabled) {
	GuiTask::lock();
	lv_subject_set_int(&m_wifi_enabled_subject, enabled ? 1 : 0);
	GuiTask::unlock();
	return WiFiManager::getInstance().setEnabled(enabled);
}
bool ConnectivityManager::isWiFiEnabled() const {
	return WiFiManager::getInstance().isEnabled();
}
esp_err_t ConnectivityManager::startHotspot(const char* s, const char* p, int c, int m, bool h, wifi_auth_mode_t auth, int8_t tx) {
	return HotspotManager::getInstance().start(s, p, c, m, h, auth, tx);
}
esp_err_t ConnectivityManager::stopHotspot() {
	return HotspotManager::getInstance().stop();
}
bool ConnectivityManager::isHotspotEnabled() const {
	return HotspotManager::getInstance().isEnabled();
}
std::vector<HotspotManager::ClientInfo>
ConnectivityManager::getHotspotClientsList() const {
	return HotspotManager::getInstance().getConnectedClients();
}
esp_err_t ConnectivityManager::enableBluetooth(bool e) {
	return BluetoothManager::getInstance().enable(e);
}
bool ConnectivityManager::isBluetoothEnabled() const {
	return BluetoothManager::getInstance().isEnabled();
}

} // namespace System
