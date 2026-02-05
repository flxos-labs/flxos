#include "WiFiManager.hpp"
#include "Observable.hpp"
#include "core/common/Logger.hpp"
#include "core/connectivity/ConnectivityManager.hpp"
#include "core/system/time/TimeManager.hpp"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_event_base.h"
#include "esp_netif.h"
#include "esp_netif_types.h"
#include "esp_wifi.h"
#include "esp_wifi_types_generic.h"
#include <cstdint>
#include <cstring>
#include <string_view>

static constexpr std::string_view TAG = "WiFiManager";

namespace System {

WiFiManager& WiFiManager::getInstance() {
	static WiFiManager instance;
	return instance;
}

esp_err_t WiFiManager::init(Observable<int32_t>* connected_subject, StringObservable* ssid_subject, StringObservable* ip_subject, Observable<int32_t>* status_subject) {
	if (m_is_init) {
		return ESP_OK;
	}

	Log::info(TAG, "Initializing WiFi manager...");

	m_connected_subject = connected_subject;
	m_ssid_subject = ssid_subject;
	m_ip_subject = ip_subject;
	m_status_subject = status_subject;

	esp_err_t err = esp_event_handler_instance_register(
		WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, this, nullptr
	);
	if (err != ESP_OK) {
		return err;
	}

	err = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, this, nullptr);
	if (err != ESP_OK) {
		return err;
	}

	m_is_init = true;
	return ESP_OK;
}

esp_err_t WiFiManager::connect(const char* ssid, const char* password) {
	if (!m_is_enabled) {
		return ESP_ERR_INVALID_STATE;
	}

	Log::info(TAG, "Connecting to SSID: %s", ssid);
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
		setStatus(WiFiStatus::DISCONNECTED);
		return err;
	}

	err = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
	if (err != ESP_OK) {
		setStatus(WiFiStatus::DISCONNECTED);
		return err;
	}

	return esp_wifi_connect();
}

esp_err_t WiFiManager::disconnect() {
	Log::info(TAG, "Disconnecting WiFi...");
	m_should_reconnect = false;
	setStatus(WiFiStatus::DISCONNECTED);
	return esp_wifi_disconnect();
}

esp_err_t WiFiManager::setEnabled(bool enabled) {
	if (m_is_enabled == enabled) {
		return ESP_OK;
	}

	std::lock_guard<std::recursive_mutex> wifi_lock(
		ConnectivityManager::getInstance().getWifiMutex()
	);

	m_is_enabled = enabled;
	wifi_mode_t current_mode;
	esp_wifi_get_mode(&current_mode);
	bool const ap_enabled =
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
	if (!m_is_enabled) {
		return ESP_ERR_INVALID_STATE;
	}
	if (m_is_scanning) {
		return ESP_ERR_INVALID_STATE;
	}

	std::lock_guard<std::recursive_mutex> wifi_lock(
		ConnectivityManager::getInstance().getWifiMutex()
	);

	Log::info(TAG, "Scanning for WiFi networks...");
	m_is_scanning = true;
	m_scan_callback = callback;
	wifi_scan_config_t scan_config = {};
	scan_config.show_hidden = false;

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
		m_is_scanning = false;
		setStatus(WiFiStatus::DISCONNECTED);
		return err;
	}

	err = esp_wifi_scan_start(&scan_config, false);
	if (err != ESP_OK) {
		m_is_scanning = false;
		m_scan_callback = nullptr;
		setStatus(m_is_enabled ? WiFiStatus::DISCONNECTED : WiFiStatus::DISABLED);
	}
	return err;
}

bool WiFiManager::isConnected() const {
	if (!m_connected_subject) {
		return false;
	}
	return m_connected_subject->get() != 0;
}

int8_t WiFiManager::getRssi() {
	wifi_ap_record_t ap_info;
	if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
		return ap_info.rssi;
	}
	return -127;
}

void WiFiManager::setStatus(WiFiStatus status) {
	if (m_status_subject) {
		m_status_subject->set(static_cast<int>(status));
	}
}

void WiFiManager::wifi_event_handler(void* arg, esp_event_base_t /*event_base*/, int32_t event_id, void* event_data) {
	auto* self = static_cast<WiFiManager*>(arg);

	if (event_id == WIFI_EVENT_STA_START) {
		self->handleStaStart();
	} else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
		self->handleStaDisconnected(event_data);
	} else if (event_id == WIFI_EVENT_STA_CONNECTED) {
		self->handleStaConnected(event_data);
	} else if (event_id == WIFI_EVENT_SCAN_DONE) {
		self->handleScanDone();
	}
}

void WiFiManager::handleStaStart() {
	Log::debug(TAG, "STA started");
	if (m_should_reconnect) {
		esp_wifi_connect();
	}
}

void WiFiManager::handleStaDisconnected(void* event_data) {
	auto* event = (wifi_event_sta_disconnected_t*)event_data;
	Log::warn(TAG, "STA disconnected, reason: %d", event->reason);

	if (m_connected_subject) {
		m_connected_subject->set(0);
	}
	if (m_ssid_subject) {
		m_ssid_subject->set("Disconnected");
	}
	if (m_ip_subject) {
		m_ip_subject->set("0.0.0.0");
	}

	bool const is_auth_failure =
		(event->reason == WIFI_REASON_AUTH_EXPIRE ||
		 event->reason == WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT ||
		 event->reason == WIFI_REASON_AUTH_FAIL ||
		 event->reason == WIFI_REASON_HANDSHAKE_TIMEOUT);

	if (is_auth_failure) {
		m_should_reconnect = false;
		setStatus(WiFiStatus::AUTH_FAILED);
	} else if (m_should_reconnect && m_retry_count < MAX_RETRIES) {
		setStatus(WiFiStatus::CONNECTING);
		esp_err_t const err = esp_wifi_connect();
		if (err == ESP_OK) {
			m_retry_count++;
		}
	} else if (m_retry_count >= MAX_RETRIES) {
		m_should_reconnect = false;
		if (event->reason == WIFI_REASON_NO_AP_FOUND) {
			setStatus(WiFiStatus::NOT_FOUND);
		} else {
			setStatus(WiFiStatus::DISCONNECTED);
		}
	} else {
		setStatus(WiFiStatus::DISCONNECTED);
	}
}

void WiFiManager::handleStaConnected(void* event_data) {
	auto* event = (wifi_event_sta_connected_t*)event_data;
	Log::info(TAG, "STA connected to SSID: %s", (char*)event->ssid);
	if (m_ssid_subject) {
		m_ssid_subject->set((char*)event->ssid);
	}
}

void WiFiManager::handleScanDone() {
	uint16_t ap_count = 0;
	esp_wifi_scan_get_ap_num(&ap_count);
	Log::info(TAG, "Scan done, found %d APs", ap_count);

	std::vector<wifi_ap_record_t> ap_records;
	if (ap_count > 0) {
		ap_records.resize(ap_count);
		esp_err_t err = esp_wifi_scan_get_ap_records(&ap_count, ap_records.data());
		if (err != ESP_OK) {
			ap_records.clear();
		}
	}

	if (m_scan_callback) {
		m_scan_callback(ap_records);
	}

	m_is_scanning = false;
	m_scan_callback = nullptr;
	setStatus(isConnected() ? WiFiStatus::CONNECTED : (m_is_enabled ? WiFiStatus::DISCONNECTED : WiFiStatus::DISABLED));
}

void WiFiManager::ip_event_handler(void* arg, esp_event_base_t /*event_base*/, int32_t event_id, void* event_data) {
	auto* self = static_cast<WiFiManager*>(arg);

	if (event_id == IP_EVENT_STA_GOT_IP) {
		auto* event = (ip_event_got_ip_t*)event_data;
		char ip_str[16];
		esp_ip4addr_ntoa(&event->ip_info.ip, ip_str, sizeof(ip_str));
		Log::info(TAG, "Got IP: %s", ip_str);

		if (self->m_ip_subject) {
			self->m_ip_subject->set(ip_str);
		}
		if (self->m_connected_subject) {
			self->m_connected_subject->set(1);
		}

		self->m_retry_count = 0;
		self->setStatus(WiFiStatus::CONNECTED);

		// Sync time now that we have internet access
		TimeManager::getInstance().syncTime();

		if (self->m_got_ip_callback) {
			self->m_got_ip_callback();
		}
	}
}

} // namespace System
