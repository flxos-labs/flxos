#pragma once

#include <flx/core/Observable.hpp>
#include <flx/core/Singleton.hpp>
#include "esp_err.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include <atomic>
#include <functional>
#include <vector>

namespace System {

enum class WiFiStatus {
	DISABLED,
	DISCONNECTED,
	SCANNING,
	CONNECTING,
	CONNECTED,
	AUTH_FAILED,
	NOT_FOUND
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
	int m_retry_count = 0;
	static constexpr int MAX_RETRIES = 5;

	ScanCallback m_scan_callback = nullptr;
	GotIPCallback m_got_ip_callback = nullptr;
};

} // namespace System
