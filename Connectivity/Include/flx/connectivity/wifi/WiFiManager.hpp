#pragma once

#include "esp_err.h"
#include "esp_event.h"
#include <Config.hpp>

#if FLXOS_WIFI_ENABLED
#include "esp_wifi.h"
#else
struct wifi_ap_record_t {
	uint8_t ssid[33];
	int8_t rssi;
	uint8_t primary;
	int authmode;
};
#endif
#include <atomic>
#include <esp_timer.h>
#include <flx/connectivity/wifi/WiFiCredentialStore.hpp>
#include <flx/core/Observable.hpp>
#include <flx/core/Singleton.hpp>
#include <functional>
#include <vector>

namespace flx::connectivity {

enum class WiFiStatus {
	RADIO_OFF = 0, ///< Radio fully disabled
	DISABLED = 0, ///< Alias for RADIO_OFF (legacy compat) — must stay == RADIO_OFF
	RADIO_ON_PENDING, ///< Enabling in progress
	DISCONNECTED, ///< Radio on, not connected
	SCANNING, ///< Active scan running
	CONNECTING, ///< Association in progress
	CONNECTED, ///< Got IP
	AUTH_FAILED, ///< Wrong password
	NOT_FOUND, ///< SSID not visible after retries
};

class WiFiManager : public flx::Singleton<WiFiManager> {
	friend class flx::Singleton<WiFiManager>;

public:

	esp_err_t init(flx::Observable<int32_t>* connected_subject, flx::StringObservable* ssid_subject, flx::StringObservable* ip_subject, flx::Observable<int32_t>* status_subject);
	esp_err_t connect(const char* ssid, const char* password);
	esp_err_t disconnect();
	bool isConnected() const;
	static int8_t getRssi();

	esp_err_t setEnabled(bool enabled);
	bool isEnabled() const { return m_is_enabled; }

	using ScanCallback =
		std::function<void(const std::vector<wifi_ap_record_t>&)>;
	esp_err_t scan(ScanCallback callback);

	using GotIPCallback = std::function<void()>;
	void setOnGotIPCallback(GotIPCallback callback) {
		m_got_ip_callback = callback;
	}

	/// Scan for visible networks and connect to the highest-priority known network.
	/// Used for smart auto-connect after boot and after disconnections.
	esp_err_t connectBestKnownNetwork();

private:

	WiFiManager() = default;
	~WiFiManager() = default;

	void setStatus(WiFiStatus status);

	static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
	static void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

	void handleStaStart();
	void handleStaDisconnected(void* event_data);
	void handleStaConnected(void* event_data);
	void handleScanDone();

	flx::Observable<int32_t>* m_connected_subject = nullptr;
	flx::StringObservable* m_ssid_subject = nullptr;
	flx::StringObservable* m_ip_subject = nullptr;
	flx::Observable<int32_t>* m_status_subject = nullptr;

	bool m_is_init = false;
	bool m_is_enabled = false;
	std::atomic<bool> m_is_scanning {false};
	bool m_should_reconnect = false;
	bool m_manual_disconnect = false; ///< Set on explicit user disconnect; suppresses auto-reconnect
	int m_retry_count = 0;
	static constexpr int MAX_RETRIES = 5;
	uint32_t m_retry_delay_ms = 1000;
	static constexpr uint32_t MAX_RETRY_DELAY_MS = 16000;
	esp_timer_handle_t m_retry_timer = nullptr;

	// Credential carried across the event-handler boundary for best-known connect
	std::string m_pending_ssid;
	std::string m_pending_password;

	ScanCallback m_scan_callback = nullptr;
	GotIPCallback m_got_ip_callback = nullptr;
};

} // namespace flx::connectivity
