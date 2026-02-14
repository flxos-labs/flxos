#pragma once

#include "esp_err.h"
#include "esp_event.h"
#include "esp_wifi_types.h"
#include <flx/core/Observable.hpp>
#include <flx/core/Singleton.hpp>
extern "C" {
#include "dhcpserver/dhcpserver_hostname.h"
}
#include <mutex>
#include <string>
#include <vector>

namespace System {

class HotspotManager : public flx::Singleton<HotspotManager> {
	friend class flx::Singleton<HotspotManager>;

public:

	esp_err_t init(flx::Observable<int32_t>* enabled_subject, flx::Observable<int32_t>* client_count_subject);
	esp_err_t start(const char* ssid, const char* password, int channel = 1, int max_connections = 4, bool hidden = false, wifi_auth_mode_t auth_mode = WIFI_AUTH_WPA2_PSK, int8_t max_tx_power = 80);
	esp_err_t stop();
	static bool isEnabled();
	int getClientCount() const;

	esp_err_t setNatEnabled(bool enabled, bool syncDns = false);
	bool isNatEnabled() const { return m_nat_enabled; }

	uint64_t getBytesSent() const { return m_bytes_sent; }
	uint64_t getBytesReceived() const { return m_bytes_received; }
	void resetUsage() {
		m_bytes_sent = 0;
		m_bytes_received = 0;
	}

	uint32_t getUploadSpeed() const { return m_upload_speed; }
	uint32_t getDownloadSpeed() const { return m_download_speed; }
	uint32_t getUptime() const;

	void addBytesSent(uint32_t bytes);
	void addBytesReceived(uint32_t bytes);
	void* getStaNetif() const { return m_sta_netif; }
	void* getOriginalInput() const { return m_original_input; }
	void* getOriginalLinkoutput() const { return m_original_linkoutput; }
	void initByteCounter();
	void initApHook();
	void processIncomingPacket(
		void* p
	); // Using void* to avoid pbuf header requirement in hpp
	static void startUsageTimer();

	struct ClientInfo {
		uint8_t mac[6] {};
		int aid {};
		std::string hostname {};
		std::string ip {};
		int8_t rssi {};
		uint32_t connected_duration {}; // Seconds
		uint64_t connection_timestamp {};
	};
	std::vector<ClientInfo> getConnectedClients();

	void setAutoShutdownTimeout(uint32_t seconds) {
		m_auto_shutdown_timeout = seconds;
	}
	uint32_t getAutoShutdownTimeout() const { return m_auto_shutdown_timeout; }

private:

	HotspotManager() = default;
	~HotspotManager() = default;

	void updateClientHostname(uint8_t* mac, const std::string& hostname);
	void checkAutoShutdown();

	static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

	flx::Observable<int32_t>* m_enabled_subject = nullptr;
	flx::Observable<int32_t>* m_client_count_subject = nullptr;

	bool m_is_init = false;
	bool m_nat_enabled = true;
	int m_client_count = 0;
	uint64_t m_bytes_sent = 0;
	uint64_t m_bytes_received = 0;

	uint32_t m_upload_speed = 0; // bytes per second
	uint32_t m_download_speed = 0; // bytes per second
	uint64_t m_last_update_time = 0;
	uint64_t m_last_bytes_sent = 0;
	uint64_t m_last_bytes_received = 0;
	uint64_t m_start_time = 0;

	uint32_t m_auto_shutdown_timeout = 300; // 5 minutes default (0 to disable)
	uint64_t m_last_client_time = 0;

	void* m_sta_netif = nullptr; // Internal lwIP netif pointer
	void* m_original_input = nullptr;
	void* m_original_linkoutput = nullptr;

	std::string m_current_ssid {};
	std::vector<ClientInfo> m_clients {};
	mutable std::mutex m_mutex {};
};

} // namespace System
