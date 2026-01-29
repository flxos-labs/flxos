#include "WiFiManager.hpp"
#include "core/connectivity/ConnectivityManager.hpp"
#include "core/system/TimeManager.hpp"
#include "core/tasks/gui/GuiTask.hpp"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include <cstring>

static const char* TAG = "WiFiManager";

namespace System {

WiFiManager& WiFiManager::getInstance() {
	static WiFiManager instance;
	return instance;
}

esp_err_t WiFiManager::init(lv_subject_t* connected_subject, lv_subject_t* ssid_subject, lv_subject_t* ip_subject, lv_subject_t* status_subject) {
	if (m_is_init)
		return ESP_OK;

	m_connected_subject = connected_subject;
	m_ssid_subject = ssid_subject;
	m_ip_subject = ip_subject;
	m_status_subject = status_subject;

	esp_err_t err = esp_event_handler_instance_register(
		WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, this, nullptr
	);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to register WiFi event handler: %s", esp_err_to_name(err));
		return err;
	}

	err = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, this, nullptr);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to register IP event handler: %s", esp_err_to_name(err));
		return err;
	}

	m_is_init = true;
	return ESP_OK;
}

esp_err_t WiFiManager::connect(const char* ssid, const char* password) {
	if (!m_is_enabled)
		return ESP_ERR_INVALID_STATE;

	ESP_LOGI(TAG, "Connecting to WiFi: %s", ssid);
	m_should_reconnect = true;
	m_retry_count = 0;
	setStatus(WiFiStatus::CONNECTING);

	std::lock_guard<std::recursive_mutex> wifi_lock(
		ConnectivityManager::getInstance().getWifiMutex()
	);

	// Ensure any previous connection attempt is stopped
	esp_wifi_disconnect();

	wifi_config_t wifi_config = {};
	strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
	if (password) {
		strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
	}

	wifi_mode_t current_mode;
	esp_wifi_get_mode(&current_mode);
	wifi_mode_t target_mode;
	if (current_mode == WIFI_MODE_AP || current_mode == WIFI_MODE_APSTA) {
		target_mode = WIFI_MODE_APSTA;
	} else {
		target_mode = WIFI_MODE_STA;
	}

	esp_err_t err = ConnectivityManager::getInstance().setWifiMode(target_mode);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to set mode: %s", esp_err_to_name(err));
		setStatus(WiFiStatus::DISCONNECTED);
		return err;
	}

	err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to set config: %s", esp_err_to_name(err));
		setStatus(WiFiStatus::DISCONNECTED);
		return err;
	}

	return esp_wifi_connect();
}

esp_err_t WiFiManager::disconnect() {
	ESP_LOGI(TAG, "Disconnecting from WiFi");
	m_should_reconnect = false;
	setStatus(WiFiStatus::DISCONNECTED);
	return esp_wifi_disconnect();
}

esp_err_t WiFiManager::setEnabled(bool enabled) {
	if (m_is_enabled == enabled)
		return ESP_OK;

	ESP_LOGI(TAG, "WiFi Manager setEnabled: %d", enabled);
	std::lock_guard<std::recursive_mutex> wifi_lock(
		ConnectivityManager::getInstance().getWifiMutex()
	);

	m_is_enabled = enabled;
	wifi_mode_t current_mode;
	esp_wifi_get_mode(&current_mode);
	bool ap_enabled =
		(current_mode == WIFI_MODE_AP || current_mode == WIFI_MODE_APSTA);

	if (!enabled) {
		m_should_reconnect = false;
		esp_wifi_disconnect();
		setStatus(WiFiStatus::DISABLED);
		return ConnectivityManager::getInstance().setWifiMode(
			ap_enabled ? WIFI_MODE_AP : WIFI_MODE_NULL
		);
	} else {
		setStatus(WiFiStatus::DISCONNECTED);
		return ConnectivityManager::getInstance().setWifiMode(
			ap_enabled ? WIFI_MODE_APSTA : WIFI_MODE_STA
		);
	}
}

esp_err_t WiFiManager::scan(ScanCallback callback) {
	if (!m_is_enabled)
		return ESP_ERR_INVALID_STATE;
	if (m_is_scanning)
		return ESP_ERR_INVALID_STATE;

	std::lock_guard<std::recursive_mutex> wifi_lock(
		ConnectivityManager::getInstance().getWifiMutex()
	);

	m_is_scanning = true;
	m_scan_callback = callback;
	wifi_scan_config_t scan_config = {};
	scan_config.show_hidden = false;

	ESP_LOGI(TAG, "Starting WiFi scan...");
	setStatus(WiFiStatus::SCANNING);

	wifi_mode_t current_mode;
	esp_wifi_get_mode(&current_mode);
	wifi_mode_t target_mode;
	if (current_mode == WIFI_MODE_AP || current_mode == WIFI_MODE_APSTA) {
		target_mode = WIFI_MODE_APSTA;
	} else {
		target_mode = WIFI_MODE_STA;
	}

	esp_err_t err = ConnectivityManager::getInstance().setWifiMode(target_mode);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to set mode for scan: %s", esp_err_to_name(err));
		m_is_scanning = false;
		setStatus(WiFiStatus::DISCONNECTED);
		return err;
	}

	err = esp_wifi_scan_start(&scan_config, false);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to start scan: %s", esp_err_to_name(err));
		m_is_scanning = false;
		m_scan_callback = nullptr;
		setStatus(m_is_enabled ? WiFiStatus::DISCONNECTED : WiFiStatus::DISABLED);
	}
	return err;
}

bool WiFiManager::isConnected() const {
	GuiTask::lock();
	if (!m_connected_subject) {
		ESP_LOGW(TAG, "isConnected check: No connected subject initialized");
		GuiTask::unlock();
		return false;
	}
	bool connected = lv_subject_get_int(m_connected_subject) != 0;
	GuiTask::unlock();
	ESP_LOGD(TAG, "isConnected check: %s", connected ? "TRUE" : "FALSE");
	return connected;
}

int8_t WiFiManager::getRssi() const {
	wifi_ap_record_t ap_info;
	if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
		return ap_info.rssi;
	}
	return -127;
}

void WiFiManager::setStatus(WiFiStatus status) {
	GuiTask::lock();
	if (m_status_subject) {
		lv_subject_set_int(m_status_subject, static_cast<int>(status));
	}
	GuiTask::unlock();
}

void WiFiManager::wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
	WiFiManager* self = static_cast<WiFiManager*>(arg);

	if (event_id == WIFI_EVENT_STA_START) {
		ESP_LOGI(TAG, "WiFi STA started");
		if (self->m_should_reconnect)
			esp_wifi_connect();
	} else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
		wifi_event_sta_disconnected_t* event =
			(wifi_event_sta_disconnected_t*)event_data;

		GuiTask::lock();
		if (self->m_connected_subject)
			lv_subject_set_int(self->m_connected_subject, 0);
		if (self->m_ssid_subject)
			lv_subject_copy_string(self->m_ssid_subject, "Disconnected");
		if (self->m_ip_subject)
			lv_subject_copy_string(self->m_ip_subject, "0.0.0.0");
		GuiTask::unlock();

		bool is_auth_failure =
			(event->reason == WIFI_REASON_AUTH_EXPIRE ||
			 event->reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT ||
			 event->reason == WIFI_REASON_AUTH_FAIL ||
			 event->reason == WIFI_REASON_HANDSHAKE_TIMEOUT);

		if (is_auth_failure) {
			ESP_LOGW(TAG, "Authentication failed (reason=%d). Stopping retries.", event->reason);
			self->m_should_reconnect = false;
			self->setStatus(WiFiStatus::AUTH_FAILED);
		} else if (self->m_should_reconnect && self->m_retry_count < MAX_RETRIES) {
			ESP_LOGI(TAG, "Disconnected from WiFi (reason=%d), retrying (%d/%d)...", event->reason, self->m_retry_count + 1, MAX_RETRIES);
			self->setStatus(WiFiStatus::CONNECTING);
			esp_err_t err = esp_wifi_connect();
			if (err == ESP_OK) {
				self->m_retry_count++;
			} else {
				ESP_LOGE(TAG, "Retry esp_wifi_connect failed: %s", esp_err_to_name(err));
			}
		} else if (self->m_retry_count >= MAX_RETRIES) {
			ESP_LOGW(TAG, "Max retries reached. Stopping reconnection.");
			self->m_should_reconnect = false;

			if (event->reason == WIFI_REASON_NO_AP_FOUND) {
				self->setStatus(WiFiStatus::NOT_FOUND);
			} else {
				self->setStatus(WiFiStatus::DISCONNECTED);
			}
		} else {
			ESP_LOGI(TAG, "Disconnected from WiFi (reason=%d), no regrouping requested or enabled.", event->reason);
			self->setStatus(WiFiStatus::DISCONNECTED);
		}
	} else if (event_id == WIFI_EVENT_STA_CONNECTED) {
		wifi_event_sta_connected_t* event =
			(wifi_event_sta_connected_t*)event_data;
		ESP_LOGI(TAG, "WiFi connected to SSID: %s", (char*)event->ssid);
		GuiTask::lock();
		if (self->m_ssid_subject)
			lv_subject_copy_string(self->m_ssid_subject, (char*)event->ssid);
		GuiTask::unlock();
	} else if (event_id == WIFI_EVENT_SCAN_DONE) {
		ESP_LOGI(TAG, "WiFi scan done");
		uint16_t ap_count = 0;
		esp_wifi_scan_get_ap_num(&ap_count);
		if (ap_count > 0) {
			std::vector<wifi_ap_record_t> ap_records(ap_count);
			esp_err_t err =
				esp_wifi_scan_get_ap_records(&ap_count, ap_records.data());
			if (err == ESP_OK && self->m_scan_callback) {
				self->m_scan_callback(ap_records);
			} else if (self->m_scan_callback) {
				self->m_scan_callback({});
			}
		} else {
			if (self->m_scan_callback) {
				self->m_scan_callback({});
			}
		}
		self->m_is_scanning = false;
		self->m_scan_callback = nullptr;
		self->setStatus(self->isConnected() ? WiFiStatus::CONNECTED : (self->m_is_enabled ? WiFiStatus::DISCONNECTED : WiFiStatus::DISABLED));
	}
}

void WiFiManager::ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
	WiFiManager* self = static_cast<WiFiManager*>(arg);

	if (event_id == IP_EVENT_STA_GOT_IP) {
		ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
		char ip_str[16];
		esp_ip4addr_ntoa(&event->ip_info.ip, ip_str, sizeof(ip_str));

		GuiTask::lock();
		if (self->m_ip_subject)
			lv_subject_copy_string(self->m_ip_subject, ip_str);
		if (self->m_connected_subject)
			lv_subject_set_int(self->m_connected_subject, 1);
		GuiTask::unlock();

		self->m_retry_count = 0;
		self->setStatus(WiFiStatus::CONNECTED);
		ESP_LOGI(TAG, "Got IP: %s", ip_str);

		// Sync time now that we have internet access
		TimeManager::getInstance().syncTime();

		if (self->m_got_ip_callback) {
			self->m_got_ip_callback();
		}
	}
}

} // namespace System
