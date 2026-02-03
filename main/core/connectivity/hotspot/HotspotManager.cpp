#include "HotspotManager.hpp"
#include "core/common/Logger.hpp"
#include "core/connectivity/ConnectivityManager.hpp"
#include "core/connectivity/wifi/WiFiManager.hpp"
#include "esp_timer.h"
#include "esp_wifi.h"
#include <cstring>
#include <string_view>

#include "esp_netif.h"

extern "C" {
#include "lwip/lwip_napt.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"

// Internal IDF API to get lwIP netif from esp_netif
struct netif* esp_netif_get_netif_impl(esp_netif_t* esp_netif);
}

static constexpr std::string_view TAG = "HotspotManager";

namespace System {

// Static hooks to capture traffic
static err_t netif_input_hook(struct pbuf* p, struct netif* netif) {
	HotspotManager& hm = HotspotManager::getInstance();
	if (netif == (struct netif*)hm.getStaNetif() && p != NULL) {
		hm.addBytesReceived(p->tot_len);
	}

	netif_input_fn original = (netif_input_fn)hm.getOriginalInput();
	if (original)
		return original(p, netif);
	return ERR_VAL;
}

static err_t netif_linkoutput_hook(struct netif* netif, struct pbuf* p) {
	HotspotManager& hm = HotspotManager::getInstance();
	if (netif == (struct netif*)hm.getStaNetif() && p != NULL) {
		hm.addBytesSent(p->tot_len);
	}

	netif_linkoutput_fn original =
		(netif_linkoutput_fn)hm.getOriginalLinkoutput();
	if (original)
		return original(netif, p);
	return ERR_IF;
}

HotspotManager& HotspotManager::getInstance() {
	static HotspotManager instance;
	return instance;
}

void HotspotManager::addBytesSent(uint32_t bytes) { m_bytes_sent += bytes; }

void HotspotManager::addBytesReceived(uint32_t bytes) {
	m_bytes_received += bytes;
}

void HotspotManager::initByteCounter() {
	if (m_sta_netif != nullptr)
		return; // Already hooked

	esp_netif_t* sta_netif_handle =
		esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
	if (sta_netif_handle) {
		struct netif* lwip_netif = esp_netif_get_netif_impl(sta_netif_handle);
		if (lwip_netif) {
			m_sta_netif = lwip_netif;
			m_original_input = (void*)lwip_netif->input;
			lwip_netif->input = netif_input_hook;

			m_original_linkoutput = (void*)lwip_netif->linkoutput;
			lwip_netif->linkoutput = netif_linkoutput_hook;
		}
	}
}

void HotspotManager::initApHook() {
	// Now using custom dhcpserver component, no need for manual RX hook for
	// hostnames
}

void HotspotManager::processIncomingPacket(void* ptr) {
	// No-op, hostname captured in custom dhcpserver
}

void HotspotManager::updateClientHostname(uint8_t* mac, const std::string& hostname) {
	std::lock_guard<std::mutex> lock(m_mutex);
	for (auto& client: m_clients) {
		if (memcmp(client.mac, mac, 6) == 0) {
			Log::info(TAG, "Updated client hostname: %s", hostname.c_str());
			client.hostname = hostname;
			break;
		}
	}
}

std::vector<HotspotManager::ClientInfo> HotspotManager::getConnectedClients() {
	std::lock_guard<std::mutex> lock(m_mutex);

	// Refresh hostnames and IPs from custom dhcpserver
	static dhcp_lease_info_t leases_buf[8];
	int lease_count = dhcps_get_active_leases(leases_buf, 8);

	// Get STA list from WiFi driver for RSSI
	wifi_sta_list_t sta_list;
	esp_wifi_ap_get_sta_list(&sta_list);

	uint64_t now = esp_timer_get_time() / 1000000;

	for (auto& client: m_clients) {
		// Update IP and Hostname
		for (int i = 0; i < lease_count; i++) {
			if (memcmp(client.mac, leases_buf[i].mac, 6) == 0) {
				if (strlen(leases_buf[i].hostname) > 0) {
					client.hostname = leases_buf[i].hostname;
				}
				ip4_addr_t addr;
				addr.addr = leases_buf[i].ip;
				char ip_str[16];
				esp_ip4addr_ntoa((const esp_ip4_addr_t*)&addr, ip_str, sizeof(ip_str));
				client.ip = ip_str;
				break;
			}
		}

		// Update RSSI
		for (int i = 0; i < sta_list.num; i++) {
			if (memcmp(client.mac, sta_list.sta[i].mac, 6) == 0) {
				client.rssi = sta_list.sta[i].rssi;
				break;
			}
		}

		// Update Duration
		client.connected_duration = (uint32_t)(now - client.connection_timestamp);
	}

	return m_clients;
}

esp_err_t HotspotManager::init(Observable<int32_t>* enabled_subject, Observable<int32_t>* client_count_subject) {
	if (m_is_init)
		return ESP_OK;

	Log::info(TAG, "Initializing Hotspot manager...");
	m_enabled_subject = enabled_subject;
	m_client_count_subject = client_count_subject;

	esp_err_t err = esp_event_handler_instance_register(
		WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, this, nullptr
	);
	if (err != ESP_OK) {
		return err;
	}

	WiFiManager::getInstance().setOnGotIPCallback([this]() {
		initByteCounter(); // Ensure byte counter is initialized when STA is ready
		if (m_nat_enabled && isEnabled()) {
			// Re-apply NAT settings to ensure everything is synced with the new
			// uplink IP
			setNatEnabled(true);

			// Sync DNS from STA to AP
			esp_netif_dns_info_t dns;
			esp_netif_t* sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
			esp_netif_t* ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
			if (sta_netif && ap_netif) {
				if (esp_netif_get_dns_info(sta_netif, ESP_NETIF_DNS_MAIN, &dns) ==
					ESP_OK) {
					esp_netif_dhcps_stop(ap_netif);
					esp_netif_set_dns_info(ap_netif, ESP_NETIF_DNS_MAIN, &dns);
					esp_netif_dhcps_start(ap_netif);
				}
			}
		}
	});

	m_is_init = true;
	return ESP_OK;
}

uint32_t HotspotManager::getUptime() const {
	if (!isEnabled() || m_start_time == 0)
		return 0;
	return (uint32_t)((esp_timer_get_time() - m_start_time) / 1000000); // Seconds
}

void HotspotManager::startUsageTimer() {
	// Create timer to update usage (every 2 seconds)
	// Uses esp_timer instead of lv_timer for headless compatibility
	esp_timer_handle_t timer;
	esp_timer_create_args_t timer_args = {
		.callback = [](void* arg) {
			HotspotManager& self = HotspotManager::getInstance();
			if (self.isEnabled()) {
				uint64_t now = esp_timer_get_time();
				uint64_t dt_us = now - self.m_last_update_time;

				if (dt_us > 0 && self.m_last_update_time > 0) {
					float dt_s = (float)dt_us / 1000000.0f;
					self.m_upload_speed =
						(uint32_t)((float)(self.m_bytes_sent - self.m_last_bytes_sent) /
								   dt_s);
					self.m_download_speed =
						(uint32_t)((float)(self.m_bytes_received -
										   self.m_last_bytes_received) /
								   dt_s);
				}

				self.m_last_update_time = now;
				self.m_last_bytes_sent = self.m_bytes_sent;
				self.m_last_bytes_received = self.m_bytes_received;

				auto& cm = ConnectivityManager::getInstance();
				cm.getHotspotUsageSentSubject().set((int32_t)(self.getBytesSent() / 1024));
				cm.getHotspotUsageReceivedSubject().set((int32_t)(self.getBytesReceived() / 1024));
				cm.getHotspotUploadSpeedSubject().set((int32_t)(self.getUploadSpeed() / 1024));
				cm.getHotspotDownloadSpeedSubject().set((int32_t)(self.getDownloadSpeed() / 1024));
				cm.getHotspotUptimeSubject().set((int32_t)self.getUptime());

				self.checkAutoShutdown();
			}
		},
		.arg = nullptr,
		.dispatch_method = ESP_TIMER_TASK,
		.name = "hotspot_usage",
		.skip_unhandled_events = true,
	};
	esp_timer_create(&timer_args, &timer);
	esp_timer_start_periodic(timer, 2000000); // 2 seconds in microseconds
}

void HotspotManager::checkAutoShutdown() {
	if (m_auto_shutdown_timeout == 0)
		return;

	uint64_t now = esp_timer_get_time() / 1000000;
	if (m_client_count == 0 && m_last_client_time > 0) {
		if ((now - m_last_client_time) >= m_auto_shutdown_timeout) {
			Log::info(TAG, "Auto-shutdown triggered (no clients for %lu seconds)", m_auto_shutdown_timeout);
			stop();
		}
	}
}

esp_err_t HotspotManager::start(const char* ssid, const char* password, int channel, int max_connections, bool hidden, wifi_auth_mode_t auth_mode, int8_t max_tx_power) {
	Log::info(TAG, "Starting hotspot SSID: %s", ssid);
	if (!ssid || strlen(ssid) == 0 || strlen(ssid) > 32) {
		return ESP_ERR_INVALID_ARG;
	}

	if (auth_mode != WIFI_AUTH_OPEN && (!password || strlen(password) < 8)) {
		return ESP_ERR_INVALID_ARG;
	}

	std::lock_guard<std::recursive_mutex> wifi_lock(
		ConnectivityManager::getInstance().getWifiMutex()
	);

	wifi_config_t wifi_config = {};
	strncpy((char*)wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid));
	wifi_config.ap.ssid_len = strlen(ssid);
	wifi_config.ap.max_connection = max_connections;
	wifi_config.ap.authmode = auth_mode;
	wifi_config.ap.channel = channel;
	wifi_config.ap.ssid_hidden = hidden ? 1 : 0;

	if (auth_mode != WIFI_AUTH_OPEN && password) {
		strncpy((char*)wifi_config.ap.password, password, sizeof(wifi_config.ap.password));
	}

	// Determine target mode to avoid killing existing station connection
	wifi_mode_t current_mode;
	esp_wifi_get_mode(&current_mode);

	wifi_mode_t target_mode;
	if (current_mode == WIFI_MODE_STA || current_mode == WIFI_MODE_APSTA) {
		target_mode = WIFI_MODE_APSTA;
	} else {
		target_mode = WIFI_MODE_AP;
	}

	esp_err_t err = ConnectivityManager::getInstance().setWifiMode(target_mode);
	if (err != ESP_OK) {
		return err;
	}

	err = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
	if (err != ESP_OK) {
		return err;
	}

	esp_wifi_set_max_tx_power(max_tx_power);

	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_current_ssid = ssid;
		m_start_time = esp_timer_get_time();
		m_last_update_time = m_start_time;
		m_last_bytes_sent = m_bytes_sent;
		m_last_bytes_received = m_bytes_received;
		m_last_client_time = m_start_time / 1000000;
	}
	return ESP_OK;
}

esp_err_t HotspotManager::stop() {
	Log::info(TAG, "Stopping hotspot...");
	std::lock_guard<std::recursive_mutex> wifi_lock(
		ConnectivityManager::getInstance().getWifiMutex()
	);

	wifi_mode_t current_mode;
	esp_wifi_get_mode(&current_mode);

	esp_err_t err = ESP_OK;
	if (current_mode == WIFI_MODE_APSTA) {
		err = ConnectivityManager::getInstance().setWifiMode(WIFI_MODE_STA);
	} else if (current_mode == WIFI_MODE_AP) {
		err = ConnectivityManager::getInstance().setWifiMode(WIFI_MODE_NULL);
	}

	if (err != ESP_OK) {
	}

	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_clients.clear();
		m_start_time = 0;
		m_upload_speed = 0;
		m_download_speed = 0;
		m_last_client_time = 0;
	}
	return err;
}

bool HotspotManager::isEnabled() const {
	wifi_mode_t mode;
	if (esp_wifi_get_mode(&mode) != ESP_OK)
		return false;
	return (mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA);
}

int HotspotManager::getClientCount() {
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_client_count;
}

esp_err_t HotspotManager::setNatEnabled(bool enabled) {
	m_nat_enabled = enabled;
	esp_netif_t* ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
	if (!ap_netif) {
		return ESP_ERR_NOT_FOUND;
	}

	if (isEnabled() && enabled) {
		esp_netif_ip_info_t ip_info;
		esp_netif_get_ip_info(ap_netif, &ip_info);

		// Enable NAPT
		ip_napt_enable(ip_info.ip.addr, 1);

		// Configure DHCP server to offer DNS
		esp_netif_dhcps_stop(ap_netif);
		dhcps_offer_t dhcps_dns_value = OFFER_DNS;
		esp_netif_dhcps_option(ap_netif, ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER, &dhcps_dns_value, sizeof(dhcps_dns_value));

		// Set a default DNS server (Google DNS) in case STA hasn't synced one yet
		esp_netif_dns_info_t dns;
		dns.ip.type = ESP_IPADDR_TYPE_V4;
		dns.ip.u_addr.ip4.addr = esp_ip4addr_aton("8.8.8.8");
		esp_netif_set_dns_info(ap_netif, ESP_NETIF_DNS_MAIN, &dns);
		esp_netif_dhcps_start(ap_netif);

		// Disable WiFi power save for better performance/reliability when routing
		esp_wifi_set_ps(WIFI_PS_NONE);

	} else if (!enabled) {
		esp_netif_ip_info_t ip_info;
		esp_netif_get_ip_info(ap_netif, &ip_info);
		ip_napt_enable(ip_info.ip.addr, 0);

		// Restore default power save if NAT is disabled
		esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
	}
	return ESP_OK;
}

void HotspotManager::wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
	HotspotManager* self = static_cast<HotspotManager*>(arg);

	if (event_id == WIFI_EVENT_AP_START) {
		Log::info(TAG, "Hotspot AP started");
		self->initApHook();

		if (self->m_nat_enabled) {
			self->setNatEnabled(true);

			// Sync DNS from STA to AP
			esp_netif_dns_info_t dns;
			esp_netif_t* sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
			esp_netif_t* ap_netif = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
			if (sta_netif && ap_netif) {
				if (esp_netif_get_dns_info(sta_netif, ESP_NETIF_DNS_MAIN, &dns) ==
					ESP_OK) {
					esp_netif_dhcps_stop(ap_netif);
					esp_netif_set_dns_info(ap_netif, ESP_NETIF_DNS_MAIN, &dns);
					esp_netif_dhcps_start(ap_netif);
				}
			}
		}

		if (self->m_enabled_subject)
			self->m_enabled_subject->set(1);
	} else if (event_id == WIFI_EVENT_AP_STOP) {
		Log::info(TAG, "Hotspot AP stopped");
		{
			std::lock_guard<std::mutex> lock(self->m_mutex);
			self->m_client_count = 0;
			self->m_clients.clear();
			self->m_last_client_time = 0;
		}
		if (self->m_client_count_subject)
			self->m_client_count_subject->set(0);
		if (self->m_enabled_subject)
			self->m_enabled_subject->set(0);
	} else if (event_id == WIFI_EVENT_AP_STACONNECTED) {
		wifi_event_ap_staconnected_t* event =
			(wifi_event_ap_staconnected_t*)event_data;
		Log::info(TAG, "Client connected to Hotspot, AID=%d", event->aid);

		{
			std::lock_guard<std::mutex> lock(self->m_mutex);
			ClientInfo info;
			memcpy(info.mac, event->mac, 6);
			info.aid = event->aid;
			info.rssi = 0;
			info.connection_timestamp = esp_timer_get_time() / 1000000;
			info.connected_duration = 0;
			self->m_clients.push_back(info);
			self->m_client_count++;
			self->m_last_client_time = 0; // Reset shutdown timer
		}

		if (self->m_client_count_subject)
			self->m_client_count_subject->set(self->m_client_count);
	} else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
		wifi_event_ap_stadisconnected_t* event =
			(wifi_event_ap_stadisconnected_t*)event_data;
		Log::info(TAG, "Client disconnected from Hotspot, AID=%d", event->aid);

		{
			std::lock_guard<std::mutex> lock(self->m_mutex);
			for (auto it = self->m_clients.begin(); it != self->m_clients.end();
				 ++it) {
				if (memcmp(it->mac, event->mac, 6) == 0) {
					self->m_clients.erase(it);
					break;
				}
			}
			if (self->m_client_count > 0)
				self->m_client_count--;
			if (self->m_client_count == 0) {
				self->m_last_client_time = esp_timer_get_time() / 1000000;
			}
		}

		if (self->m_client_count_subject)
			self->m_client_count_subject->set(self->m_client_count);
	}
}

} // namespace System
