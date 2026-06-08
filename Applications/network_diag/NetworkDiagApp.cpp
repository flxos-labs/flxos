#include "NetworkDiagApp.hpp"
#include <algorithm>
#include <cstring>
#include <ctime>
#include <esp_mac.h>
#include <esp_netif.h>
#include <esp_timer.h>
#include <esp_wifi.h>
#include <flx/connectivity/ConnectivityManager.hpp>
#include <flx/connectivity/wifi/WiFiCredentialStore.hpp>
#include <flx/connectivity/wifi/WiFiEvents.hpp>
#include <flx/connectivity/wifi/WiFiManager.hpp>
#include <flx/core/EventBus.hpp>
#include <flx/core/Logger.hpp>
#include <flx/ui/theming/StyleUtils.hpp>
#include <flx/ui/theming/layout_constants/LayoutConstants.hpp>
#include <flx/ui/theming/ui_constants/UiConstants.hpp>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <layouts/flex/lv_flex.h>
#include <lwip/dns.h>
#include <lwip/netdb.h>
#include <lwip/sockets.h>
#include <ping/ping_sock.h>
#include <widgets/bar/lv_bar.h>
#include <widgets/button/lv_button.h>
#include <widgets/label/lv_label.h>
#include <widgets/table/lv_table.h>
#include <widgets/tabview/lv_tabview.h>
#include <widgets/textarea/lv_textarea.h>

static constexpr std::string_view TAG = "NetworkDiagApp";
static constexpr uint32_t UPDATE_INTERVAL_MS = 2000;

using namespace flx::apps;
using namespace flx::connectivity;

namespace System::Apps {

std::vector<ConnectionEvent> NetworkDiagApp::s_history;
std::mutex NetworkDiagApp::s_history_mutex;

const AppManifest NetworkDiagApp::manifest = {
	.appId = "com.flxos.networkdiag",
	.appName = "Network Diag",
	.appIcon = LV_SYMBOL_WIFI,
	.appVersionName = "0.1.0",
	.appVersionCode = 1,
	.category = AppCategory::System,
	.flags = AppFlags::SingleInstance,
	.location = AppLocation::internal(),
	.description = "Advanced WiFi network diagnostics and utility tools",
	.sortPriority = 35,
	.requiredServices = {},
	.supportedMimeTypes = {},
	.urlSchemes = {},
	.createApp = []() -> std::shared_ptr<App> { return std::make_shared<NetworkDiagApp>(); }};

NetworkDiagApp::NetworkDiagApp() {
	m_pending_results.clear();
}

bool NetworkDiagApp::onStart() {
	Log::info(TAG, "App started");
	// Subscribe to event bus for connectivity history
	flx::core::EventBus::getInstance().subscribe("connectivity.wifi", [](const std::string& /*event*/, const flx::core::Bundle& data) {
		int32_t ev = data.getInt32("event");
		std::string ssid = data.getString("ssid");
		std::string ev_type = "EVENT";
		if (ev == static_cast<int32_t>(WiFiEvent::Connected)) ev_type = "CONNECTED";
		else if (ev == static_cast<int32_t>(WiFiEvent::Disconnected))
			ev_type = "DISCONNECTED";
		else if (ev == static_cast<int32_t>(WiFiEvent::Connecting))
			ev_type = "CONNECTING";
		else if (ev == static_cast<int32_t>(WiFiEvent::AuthFailed))
			ev_type = "AUTH_FAILED";
		else if (ev == static_cast<int32_t>(WiFiEvent::NotFound))
			ev_type = "NOT_FOUND";
		else
			return;

		addHistoryEvent(ssid, ev_type);
	});
	return true;
}

bool NetworkDiagApp::onResume() {
	Log::debug(TAG, "App resumed");
	updateStatusTab();
	updateSavedTab();
	updateChannelsTab();
	updateHistoryTab();
	return true;
}

void NetworkDiagApp::onPause() {}

void NetworkDiagApp::onStop() {
	m_tabview = nullptr;
	m_ssid_label = nullptr;
	m_ip_label = nullptr;
	m_rssi_label = nullptr;
	m_rssi_bar = nullptr;
	m_gateway_label = nullptr;
	m_dns_label = nullptr;
	m_mac_label = nullptr;
	m_channel_label = nullptr;
	m_saved_list = nullptr;
	m_channels_table = nullptr;
	m_target_ta = nullptr;
	m_ping_btn = nullptr;
	m_dns_btn = nullptr;
	m_results_ta = nullptr;
	m_history_table = nullptr;
}

void NetworkDiagApp::update() {
	if (isActive() && m_tabview) {
		// Flush pending background task logs to text area
		std::vector<std::string> logs;
		{
			std::lock_guard<std::mutex> lock(m_results_mutex);
			logs = std::move(m_pending_results);
			m_pending_results.clear();
		}
		for (const auto& log: logs) {
			if (m_results_ta) {
				lv_textarea_add_text(m_results_ta, log.c_str());
			}
		}

		uint32_t const now = esp_timer_get_time() / 1000;
		if (now - m_last_update >= UPDATE_INTERVAL_MS) {
			updateStatusTab();
			updateSavedTab();
			updateChannelsTab();
			updateHistoryTab();
			m_last_update = now;
		}
	}
}

void NetworkDiagApp::logResult(const std::string& line) {
	std::lock_guard<std::mutex> lock(m_results_mutex);
	m_pending_results.push_back(line);
}

void NetworkDiagApp::addHistoryEvent(const std::string& ssid, const std::string& type) {
	std::lock_guard<std::mutex> lock(s_history_mutex);
	ConnectionEvent ev;
	ev.ssid = ssid;
	ev.eventType = type;

	// Simple timestamp
	time_t now;
	time(&now);
	struct tm timeinfo;
	localtime_r(&now, &timeinfo);
	char buf[32];
	strftime(buf, sizeof(buf), "%H:%M:%S", &timeinfo);
	ev.timestamp = buf;

	s_history.insert(s_history.begin(), ev);
	if (s_history.size() > 10) {
		s_history.pop_back();
	}
}

void NetworkDiagApp::createUI(void* parent) {
	auto* container = (lv_obj_t*)parent;

	m_tabview = lv_tabview_create(container);
	lv_obj_set_size(m_tabview, lv_pct(100), lv_pct(100));
	lv_tabview_set_tab_bar_position(m_tabview, LV_DIR_TOP);
	lv_tabview_set_tab_bar_size(m_tabview, lv_dpx(UiConstants::SIZE_TAB_BAR));

	lv_obj_t* tab_status = lv_tabview_add_tab(m_tabview, "Status");
	lv_obj_t* tab_saved = lv_tabview_add_tab(m_tabview, "Saved");
	lv_obj_t* tab_channels = lv_tabview_add_tab(m_tabview, "Channels");
	lv_obj_t* tab_tools = lv_tabview_add_tab(m_tabview, "Tools");
	lv_obj_t* tab_history = lv_tabview_add_tab(m_tabview, "History");

	createStatusTab(tab_status);
	createSavedTab(tab_saved);
	createChannelsTab(tab_channels);
	createToolsTab(tab_tools);
	createHistoryTab(tab_history);

	updateStatusTab();
	updateSavedTab();
	updateChannelsTab();
	updateHistoryTab();
}

void NetworkDiagApp::createStatusTab(lv_obj_t* tab) {
	lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_all(tab, lv_dpx(UiConstants::PAD_LARGE), 0);
	lv_obj_set_style_pad_row(tab, lv_dpx(UiConstants::PAD_SMALL), 0);

	lv_obj_t* card = lv_obj_create(tab);
	lv_obj_set_width(card, lv_pct(100));
	lv_obj_set_height(card, LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_all(card, lv_dpx(UiConstants::PAD_DEFAULT), 0);
	lv_obj_set_style_pad_row(card, lv_dpx(UiConstants::PAD_SMALL), 0);
	lv_obj_set_style_radius(card, UiConstants::RADIUS_DEFAULT, 0);
	UI::StyleUtils::applyThemedBorder(card, UI::StyleUtils::ThemeColorToken::CardBorder, UiConstants::BORDER_THIN, UiConstants::OPA_30);
	UI::StyleUtils::apply_glass(card, UiConstants::GLASS_BLUR_DEFAULT);

	m_ssid_label = lv_label_create(card);
	m_ip_label = lv_label_create(card);
	m_gateway_label = lv_label_create(card);
	m_dns_label = lv_label_create(card);
	m_channel_label = lv_label_create(card);
	m_mac_label = lv_label_create(card);

	m_rssi_label = lv_label_create(card);
	m_rssi_bar = lv_bar_create(card);
	lv_obj_set_size(m_rssi_bar, lv_pct(100), lv_dpx(UiConstants::SIZE_BAR_HEIGHT));
	lv_bar_set_range(m_rssi_bar, 0, 100);
}

void NetworkDiagApp::updateStatusTab() {
	if (!m_ssid_label) return;

	bool connected = ConnectivityManager::getInstance().isWiFiConnected();
	lv_label_set_text_fmt(m_ssid_label, "SSID: %s", connected ? ConnectivityManager::getInstance().getWiFiSsidObservable().get().c_str() : "Disconnected");
	lv_label_set_text_fmt(m_ip_label, "IP: %s", connected ? ConnectivityManager::getInstance().getWiFiIpObservable().get().c_str() : "0.0.0.0");

	// Fetch network info using esp_netif
	esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
	if (netif && connected) {
		esp_netif_ip_info_t ip_info;
		esp_netif_get_ip_info(netif, &ip_info);
		char gw[16], dns[16];
		esp_ip4addr_ntoa(&ip_info.gw, gw, sizeof(gw));
		lv_label_set_text_fmt(m_gateway_label, "Gateway: %s", gw);

		esp_netif_dns_info_t dns_info;
		esp_netif_get_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns_info);
		esp_ip4addr_ntoa(&dns_info.ip.u_addr.ip4, dns, sizeof(dns));
		lv_label_set_text_fmt(m_dns_label, "DNS Server: %s", dns);
	} else {
		lv_label_set_text(m_gateway_label, "Gateway: --");
		lv_label_set_text(m_dns_label, "DNS Server: --");
	}

	uint8_t mac[6] = {};
	esp_read_mac(mac, ESP_MAC_WIFI_STA);
	lv_label_set_text_fmt(m_mac_label, "MAC Address: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	wifi_ap_record_t ap_info;
	if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK && connected) {
		lv_label_set_text_fmt(m_channel_label, "Channel: %d (Band: 2.4 GHz)", ap_info.primary);
		int rssi = ap_info.rssi;
		int percent = std::max(0, std::min(100, 2 * (rssi + 100))); // Map -100..-50 to 0..100
		lv_label_set_text_fmt(m_rssi_label, "Signal Strength: %d dBm", rssi);
		lv_bar_set_value(m_rssi_bar, percent, LV_ANIM_ON);
	} else {
		lv_label_set_text(m_channel_label, "Channel: --");
		lv_label_set_text(m_rssi_label, "Signal Strength: --");
		lv_bar_set_value(m_rssi_bar, 0, LV_ANIM_OFF);
	}
}

void NetworkDiagApp::createSavedTab(lv_obj_t* tab) {
	lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_all(tab, lv_dpx(UiConstants::PAD_LARGE), 0);

	lv_obj_t* desc = lv_label_create(tab);
	lv_label_set_text(desc, "Saved WiFi Networks");
	UI::StyleUtils::applyThemedText(desc, UI::StyleUtils::ThemeColorToken::TextPrimary);

	// Saved networks table
	m_saved_list = lv_table_create(tab);
	lv_obj_set_width(m_saved_list, lv_pct(100));
	lv_table_set_column_count(m_saved_list, 3);
	lv_table_set_column_width(m_saved_list, 0, lv_pct(50)); // SSID
	lv_table_set_column_width(m_saved_list, 1, lv_pct(25)); // Priority
	lv_table_set_column_width(m_saved_list, 2, lv_pct(25)); // Auto-Connect

	lv_table_set_cell_value(m_saved_list, 0, 0, "SSID");
	lv_table_set_cell_value(m_saved_list, 0, 1, "Prio");
	lv_table_set_cell_value(m_saved_list, 0, 2, "Auto");
}

void NetworkDiagApp::updateSavedTab() {
	if (!m_saved_list) return;

	auto saved = WiFiCredentialStore::getInstance().loadAll();
	lv_table_set_row_count(m_saved_list, saved.size() + 1);

	for (size_t i = 0; i < saved.size(); ++i) {
		const auto& cred = saved[i];
		uint32_t row = i + 1;
		lv_table_set_cell_value(m_saved_list, row, 0, cred.ssid.c_str());
		lv_table_set_cell_value_fmt(m_saved_list, row, 1, "%d", cred.priority);
		lv_table_set_cell_value(m_saved_list, row, 2, cred.autoConnect ? "Yes" : "No");
	}
}

void NetworkDiagApp::createChannelsTab(lv_obj_t* tab) {
	lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_all(tab, lv_dpx(UiConstants::PAD_LARGE), 0);

	lv_obj_t* desc = lv_label_create(tab);
	lv_label_set_text(desc, "WiFi Channel Congestion Map");
	UI::StyleUtils::applyThemedText(desc, UI::StyleUtils::ThemeColorToken::TextPrimary);

	m_channels_table = lv_table_create(tab);
	lv_obj_set_width(m_channels_table, lv_pct(100));
	lv_table_set_column_count(m_channels_table, 3);
	lv_table_set_column_width(m_channels_table, 0, lv_pct(25)); // Channel
	lv_table_set_column_width(m_channels_table, 1, lv_pct(35)); // AP Count
	lv_table_set_column_width(m_channels_table, 2, lv_pct(40)); // Congestion level

	lv_table_set_cell_value(m_channels_table, 0, 0, "Ch");
	lv_table_set_cell_value(m_channels_table, 0, 1, "APs");
	lv_table_set_cell_value(m_channels_table, 0, 2, "Congestion");
}

void NetworkDiagApp::updateChannelsTab() {
	if (!m_channels_table) return;

	uint16_t ap_count = 0;
	esp_wifi_scan_get_ap_num(&ap_count);

	std::vector<wifi_ap_record_t> records;
	if (ap_count > 0) {
		records.resize(ap_count);
		if (esp_wifi_scan_get_ap_records(&ap_count, records.data()) != ESP_OK) {
			records.clear();
		}
	}

	int counts[15] = {0};
	for (const auto& ap: records) {
		if (ap.primary >= 1 && ap.primary <= 14) {
			counts[ap.primary]++;
		}
	}

	std::vector<std::pair<int, int>> active;
	for (int i = 1; i <= 14; ++i) {
		if (counts[i] > 0) {
			active.push_back({i, counts[i]});
		}
	}

	std::sort(active.begin(), active.end(), [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
		return a.second > b.second;
	});

	lv_table_set_row_count(m_channels_table, active.size() + 1);

	for (size_t i = 0; i < active.size(); ++i) {
		int ch = active[i].first;
		int count = active[i].second;
		uint32_t row = i + 1;

		lv_table_set_cell_value_fmt(m_channels_table, row, 0, "%d", ch);
		lv_table_set_cell_value_fmt(m_channels_table, row, 1, "%d", count);

		const char* status = "Low";
		if (count >= 5) status = "High";
		else if (count >= 2)
			status = "Medium";

		lv_table_set_cell_value(m_channels_table, row, 2, status);
	}
}

void NetworkDiagApp::createToolsTab(lv_obj_t* tab) {
	lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_all(tab, lv_dpx(UiConstants::PAD_LARGE), 0);
	lv_obj_set_style_pad_row(tab, lv_dpx(UiConstants::PAD_SMALL), 0);

	lv_obj_t* desc = lv_label_create(tab);
	lv_label_set_text(desc, "Diagnostics Utilities");
	UI::StyleUtils::applyThemedText(desc, UI::StyleUtils::ThemeColorToken::TextPrimary);

	m_target_ta = lv_textarea_create(tab);
	lv_obj_set_width(m_target_ta, lv_pct(100));
	lv_textarea_set_one_line(m_target_ta, true);
	lv_textarea_set_placeholder_text(m_target_ta, "Enter IP or Hostname (e.g. 8.8.8.8)");

	lv_obj_t* btn_row = lv_obj_create(tab);
	lv_obj_set_size(btn_row, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_all(btn_row, 0, 0);
	lv_obj_set_style_bg_opa(btn_row, 0, 0);
	lv_obj_set_style_border_width(btn_row, 0, 0);

	m_ping_btn = lv_button_create(btn_row);
	lv_obj_t* p_lbl = lv_label_create(m_ping_btn);
	lv_label_set_text(p_lbl, "Ping");

	m_dns_btn = lv_button_create(btn_row);
	lv_obj_t* d_lbl = lv_label_create(m_dns_btn);
	lv_label_set_text(d_lbl, "DNS Lookup");

	m_results_ta = lv_textarea_create(tab);
	lv_obj_set_width(m_results_ta, lv_pct(100));
	lv_obj_set_flex_grow(m_results_ta, 1);
	lv_obj_remove_flag(m_results_ta, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_remove_flag(m_results_ta, LV_OBJ_FLAG_CLICK_FOCUSABLE);
	lv_textarea_set_text(m_results_ta, "Diagnostic results will appear here...\n");

	// Event callbacks
	lv_obj_add_event_cb(m_ping_btn, [](lv_event_t* e) {
		auto* app = static_cast<NetworkDiagApp*>(lv_event_get_user_data(e));
		if (app->m_ping_running || app->m_dns_running) return;

		std::string target = lv_textarea_get_text(app->m_target_ta);
		if (target.empty()) return;

		app->m_ping_target = target;
		app->m_ping_running = true;
		lv_textarea_set_text(app->m_results_ta, "");

		xTaskCreate(pingTask, "ping_task", 4096, app, 5, nullptr); }, LV_EVENT_CLICKED, this);

	lv_obj_add_event_cb(m_dns_btn, [](lv_event_t* e) {
		auto* app = static_cast<NetworkDiagApp*>(lv_event_get_user_data(e));
		if (app->m_ping_running || app->m_dns_running) return;

		std::string target = lv_textarea_get_text(app->m_target_ta);
		if (target.empty()) return;

		app->m_dns_target = target;
		app->m_dns_running = true;
		lv_textarea_set_text(app->m_results_ta, "");

		xTaskCreate(dnsTask, "dns_task", 4096, app, 5, nullptr); }, LV_EVENT_CLICKED, this);
}

void NetworkDiagApp::createHistoryTab(lv_obj_t* tab) {
	lv_obj_set_flex_flow(tab, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_all(tab, lv_dpx(UiConstants::PAD_LARGE), 0);

	lv_obj_t* desc = lv_label_create(tab);
	lv_label_set_text(desc, "Connection History");
	UI::StyleUtils::applyThemedText(desc, UI::StyleUtils::ThemeColorToken::TextPrimary);

	m_history_table = lv_table_create(tab);
	lv_obj_set_width(m_history_table, lv_pct(100));
	lv_table_set_column_count(m_history_table, 3);
	lv_table_set_column_width(m_history_table, 0, lv_pct(30)); // Time
	lv_table_set_column_width(m_history_table, 1, lv_pct(40)); // SSID
	lv_table_set_column_width(m_history_table, 2, lv_pct(30)); // Event

	lv_table_set_cell_value(m_history_table, 0, 0, "Time");
	lv_table_set_cell_value(m_history_table, 0, 1, "SSID");
	lv_table_set_cell_value(m_history_table, 0, 2, "Event");
}

void NetworkDiagApp::updateHistoryTab() {
	if (!m_history_table) return;

	std::lock_guard<std::mutex> lock(s_history_mutex);
	lv_table_set_row_count(m_history_table, s_history.size() + 1);

	for (size_t i = 0; i < s_history.size(); ++i) {
		const auto& ev = s_history[i];
		uint32_t row = i + 1;
		lv_table_set_cell_value(m_history_table, row, 0, ev.timestamp.c_str());
		lv_table_set_cell_value(m_history_table, row, 1, ev.ssid.c_str());
		lv_table_set_cell_value(m_history_table, row, 2, ev.eventType.c_str());
	}
}

void NetworkDiagApp::pingTask(void* pvParameters) {
	auto* self = static_cast<NetworkDiagApp*>(pvParameters);
	std::string target = self->m_ping_target;

	self->logResult("Pinging " + target + "...\n");

	struct addrinfo hints = {};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	struct addrinfo* res = nullptr;
	int err = getaddrinfo(target.c_str(), nullptr, &hints, &res);
	if (err != 0 || res == nullptr) {
		self->logResult("Failed to resolve host " + target + "\n");
		self->m_ping_running = false;
		vTaskDelete(nullptr);
		return;
	}

	struct sockaddr_in* ipv4 = (struct sockaddr_in*)res->ai_addr;
	ip_addr_t target_ip;
	target_ip.u_addr.ip4.addr = ipv4->sin_addr.s_addr;
	target_ip.type = IPADDR_TYPE_V4;

	char ip_str[16];
	inet_ntop(AF_INET, &(ipv4->sin_addr), ip_str, sizeof(ip_str));
	self->logResult("Resolved to IP: " + std::string(ip_str) + "\n");
	freeaddrinfo(res);

	esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
	ping_config.target_addr = target_ip;
	ping_config.count = 4;

	struct PingSessionCtx {
		NetworkDiagApp* app;
		int replies_received;
	} ctx = {self, 0};

	esp_ping_callbacks_t callbacks = {};
	callbacks.on_ping_success = [](esp_ping_handle_t h, void* ctx_void) {
		auto* session = static_cast<PingSessionCtx*>(ctx_void);
		uint32_t elapsed_time;
		uint32_t recv_len;
		ip_addr_t target_ip;
		esp_ping_get_profile(h, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));
		esp_ping_get_profile(h, ESP_PING_PROF_SIZE, &recv_len, sizeof(recv_len));
		esp_ping_get_profile(h, ESP_PING_PROF_IPADDR, &target_ip, sizeof(target_ip));

		char ip_str[16];
		inet_ntop(AF_INET, &target_ip.u_addr.ip4.addr, ip_str, sizeof(ip_str));

		session->app->logResult("Reply from " + std::string(ip_str) + ": bytes=" + std::to_string(recv_len) + " time=" + std::to_string(elapsed_time) + "ms\n");
		session->replies_received++;
	};
	callbacks.on_ping_timeout = [](esp_ping_handle_t h, void* ctx_void) {
		auto* session = static_cast<PingSessionCtx*>(ctx_void);
		session->app->logResult("Request timed out.\n");
	};
	callbacks.on_ping_end = [](esp_ping_handle_t h, void* ctx_void) {
		auto* session = static_cast<PingSessionCtx*>(ctx_void);
		uint32_t transmitted;
		uint32_t received;
		esp_ping_get_profile(h, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
		esp_ping_get_profile(h, ESP_PING_PROF_REPLY, &received, sizeof(received));
		session->app->logResult("--- Ping statistics ---\n" + std::to_string(transmitted) + " packets transmitted, " + std::to_string(received) + " received\n");
		esp_ping_delete_session(h);
		session->app->m_ping_running = false;
	};
	callbacks.cb_args = &ctx;

	esp_ping_handle_t ping_handle;
	if (esp_ping_new_session(&ping_config, &callbacks, &ping_handle) == ESP_OK) {
		esp_ping_start(ping_handle);
		while (self->m_ping_running) {
			vTaskDelay(pdMS_TO_TICKS(100));
		}
	} else {
		self->logResult("Failed to create ping session.\n");
		self->m_ping_running = false;
	}

	vTaskDelete(nullptr);
}

void NetworkDiagApp::dnsTask(void* pvParameters) {
	auto* self = static_cast<NetworkDiagApp*>(pvParameters);
	std::string host = self->m_dns_target;

	self->logResult("Resolving DNS for " + host + "...\n");

	struct addrinfo hints = {};
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	struct addrinfo* res = nullptr;
	int err = getaddrinfo(host.c_str(), nullptr, &hints, &res);
	if (err == 0 && res != nullptr) {
		struct sockaddr_in* ipv4 = (struct sockaddr_in*)res->ai_addr;
		char ip_str[16];
		inet_ntop(AF_INET, &(ipv4->sin_addr), ip_str, sizeof(ip_str));
		self->logResult("DNS Resolution Success:\nIP: " + std::string(ip_str) + "\n\n");
		freeaddrinfo(res);
	} else {
		self->logResult("DNS Resolution Failed.\n\n");
	}

	self->m_dns_running = false;
	vTaskDelete(nullptr);
}

} // namespace System::Apps
