#include <algorithm>
#include <core/lv_obj.h>
#include <core/lv_obj_event.h>
#include <core/lv_obj_pos.h>
#include <core/lv_obj_style.h>
#include <core/lv_obj_style_gen.h>
#include <core/lv_obj_tree.h>
#include <core/lv_observer.h>
#include <cstdint>
#include <display/lv_display.h>
#include <esp_err.h>
#include <esp_wifi.h>
#include <esp_wifi_types_generic.h>
#include <esp_netif.h>
#include <flx/connectivity/ConnectivityManager.hpp>
#include <flx/connectivity/wifi/WiFiCredentialStore.hpp>
#include <flx/connectivity/wifi/WiFiManager.hpp>
#include <flx/ui/GuiTask.hpp>
#include <flx/ui/common/SettingsCommon.hpp>
#include <flx/ui/theming/StyleUtils.hpp>
#include <flx/ui/theming/layout_constants/LayoutConstants.hpp>
#include <flx/ui/theming/ui_constants/UiConstants.hpp>
#include <font/lv_symbol_def.h>
#include <layouts/flex/lv_flex.h>
#include <misc/lv_area.h>
#include <misc/lv_event.h>
#include <misc/lv_types.h>
#include <widgets/button/lv_button.h>
#include <widgets/image/lv_image.h>
#include <widgets/label/lv_label.h>
#include <widgets/list/lv_list.h>
#include <widgets/switch/lv_switch.h>
#include <widgets/textarea/lv_textarea.h>

#include "WiFiSettings.hpp"

using namespace flx::ui;
using namespace flx::ui::common;

namespace System::Apps::Settings {

// Constructor removed (using inherited)

void WiFiSettings::createUI() {
	m_container = create_page_container(m_parent);
	lv_obj_set_style_pad_gap(m_container, 0, 0);

	auto& cm = flx::connectivity::ConnectivityManager::getInstance();
	m_wifiEnabledBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(cm.getWiFiEnabledObservable());
	m_wifiStatusBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(cm.getWiFiStatusObservable());
	m_wifiConnectedBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(cm.getWiFiConnectedObservable());
	m_wifiScanIntervalBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(cm.getWiFiScanIntervalObservable());
	m_wifiIpBridge = std::make_unique<flx::ui::LvglStringObserverBridge>(cm.getWiFiIpObservable());

	lv_obj_t* backBtn = nullptr;
	lv_obj_t* header = create_header(m_container, "Wi-Fi", &backBtn);
	add_back_button_event_cb(backBtn, &m_onBack);

	lv_obj_t* title = lv_obj_get_child(header, 1);
	lv_obj_set_flex_grow(title, 1);

	lv_obj_remove_flag(m_container, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t* body = lv_obj_create(m_container);
	lv_obj_set_width(body, lv_pct(100));
	lv_obj_set_flex_grow(body, 1);
	lv_obj_set_flex_flow(body, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_all(body, 0, 0);
	lv_obj_set_style_pad_gap(body, 0, 0);
	lv_obj_set_style_border_width(body, 0, 0);
	lv_obj_set_style_bg_opa(body, 0, 0);

	// Config Button
	lv_obj_t* configBtn = lv_button_create(header);
	lv_obj_set_size(configBtn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_t* configIcon = lv_image_create(configBtn);
	lv_image_set_src(configIcon, LV_SYMBOL_SETTINGS);
	lv_obj_add_event_cb(
		configBtn,
		[](lv_event_t* e) {
			auto* instance = (WiFiSettings*)lv_event_get_user_data(e);
			instance->showConfig();
		},
		LV_EVENT_CLICKED, this);

	m_wifiSwitch = lv_switch_create(header);
	lv_obj_bind_checked(
		m_wifiSwitch,
		m_wifiEnabledBridge->getSubject());

	lv_obj_add_event_cb(
		m_wifiSwitch,
		[](lv_event_t* e) {
			auto* sw = lv_event_get_target_obj(e);
			auto* instance = (WiFiSettings*)lv_event_get_user_data(e);
			bool const enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
			flx::connectivity::ConnectivityManager::getInstance().setWiFiEnabled(enabled);
			if (!enabled) {
				instance->m_isScanning = false;
				lv_obj_clean(instance->m_list);
				lv_list_add_text(instance->m_list, "Wi-Fi is disabled");
			} else {
				auto& cm = flx::connectivity::ConnectivityManager::getInstance();
				if (cm.hasSavedWiFiCredentials()) {
					cm.connectWiFi(
						cm.getSavedWiFiSsid().c_str(),
						cm.getSavedWiFiPassword().c_str(),
						false // Don't re-save, already saved
					);
					// Mark pending auto-scan for when connection completes
					instance->m_pendingAutoScan = true;
					lv_obj_clean(instance->m_list);
					lv_list_add_text(instance->m_list, "Connecting to saved network...");
				} else {
					instance->refreshScan();
				}
			}
			instance->updateStatus();
		},
		LV_EVENT_VALUE_CHANGED, this);

	lv_obj_t* statusCont = lv_obj_create(body);
	lv_obj_set_size(statusCont, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_style_pad_all(statusCont, 0, 0);
	lv_obj_set_style_pad_gap(statusCont, lv_dpx(UiConstants::PAD_SMALL), 0);
	lv_obj_set_style_pad_hor(statusCont, lv_dpx(UiConstants::PAD_MEDIUM), 0);
	lv_obj_set_style_border_width(statusCont, 0, 0);
	lv_obj_set_flex_flow(statusCont, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(statusCont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	m_statusPrefixLabel = lv_label_create(statusCont);
	lv_label_set_text(m_statusPrefixLabel, "Status: ");

	m_statusLabel = lv_label_create(statusCont);
	lv_obj_set_flex_grow(m_statusLabel, 1);
	lv_label_set_long_mode(m_statusLabel, LV_LABEL_LONG_MODE_SCROLL);

	m_infoBtn = lv_button_create(statusCont);
	lv_obj_set_size(m_infoBtn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_t* infoLabel = lv_image_create(m_infoBtn);
	lv_image_set_src(infoLabel, LV_SYMBOL_EYE_OPEN);
	lv_obj_add_flag(m_infoBtn, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_event_cb(
		m_infoBtn,
		[](lv_event_t* e) {
			auto* instance = (WiFiSettings*)lv_event_get_user_data(e);
			instance->showInfoPage();
		},
		LV_EVENT_CLICKED, this);

	lv_obj_t* refreshBtn = lv_button_create(statusCont);
	lv_obj_set_size(refreshBtn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_t* refreshLabel = lv_image_create(refreshBtn);
	lv_image_set_src(refreshLabel, LV_SYMBOL_REFRESH);
	lv_obj_add_event_cb(
		refreshBtn,
		[](lv_event_t* e) {
			auto* instance = (WiFiSettings*)lv_event_get_user_data(e);
			instance->refreshScan();
		},
		LV_EVENT_CLICKED, this);

	updateStatus();

	lv_subject_add_observer_obj(
		m_wifiIpBridge->getSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			auto* instance = (WiFiSettings*)lv_observer_get_user_data(observer);
			const char* ip = (const char*)lv_subject_get_pointer(subject);
			if (instance->m_infoIpLabel && ip) {
				lv_label_set_text(instance->m_infoIpLabel, ip);
			}
		},
		m_container, this);

	m_list = create_settings_list(body);
	lv_obj_set_flex_grow(m_list, 0);
	lv_obj_set_height(m_list, LV_SIZE_CONTENT);
	lv_obj_remove_flag(m_list, LV_OBJ_FLAG_SCROLLABLE);

	// Observer for WiFi status changes - triggers auto-scan after connection
	m_statusObserver = lv_subject_add_observer_obj(
		m_wifiConnectedBridge->getSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			auto* instance = (WiFiSettings*)lv_observer_get_user_data(observer);
			int32_t connected = lv_subject_get_int(subject);
			if (connected && instance->m_pendingAutoScan) {
				instance->m_pendingAutoScan = false;
				instance->refreshScan();
			}
			instance->updateStatus();
		},
		m_container, this);

	// Observer for scan interval setting changes
	m_scanIntervalObserver = lv_subject_add_observer_obj(
		m_wifiScanIntervalBridge->getSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			auto* instance = (WiFiSettings*)lv_observer_get_user_data(observer);
			int32_t interval = lv_subject_get_int(subject);

			// Delete existing timer if any
			if (instance->m_scanTimer != nullptr) {
				esp_timer_stop(instance->m_scanTimer);
				esp_timer_delete(instance->m_scanTimer);
				instance->m_scanTimer = nullptr;
			}

			// Create new timer if interval > 0
			if (interval > 0) {
				esp_timer_create_args_t timer_args = {
					.callback = [](void* arg) {
						auto* inst = static_cast<WiFiSettings*>(arg);
						flx::ui::GuiTask::lock();
						if (inst->m_destroying) {
							flx::ui::GuiTask::unlock();
							return;
						}
						bool should_scan = flx::connectivity::ConnectivityManager::getInstance().isWiFiEnabled() && !inst->m_isScanning;
						if (should_scan) {
							inst->refreshScan();
						}
						flx::ui::GuiTask::unlock();
					},
					.arg = instance,
					.dispatch_method = ESP_TIMER_TASK,
					.name = "wifi_scan_timer",
					.skip_unhandled_events = true,
				};
				esp_timer_create(&timer_args, &instance->m_scanTimer);
				esp_timer_start_periodic(instance->m_scanTimer, static_cast<uint64_t>(interval) * 1000000);
			}
		},
		m_container, this);

	// Trigger initial timer setup based on current setting
	int32_t initial_interval = lv_subject_get_int(m_wifiScanIntervalBridge->getSubject());
	if (initial_interval > 0) {
		esp_timer_create_args_t timer_args = {
			.callback = [](void* arg) {
				auto* inst = static_cast<WiFiSettings*>(arg);
				flx::ui::GuiTask::lock();
				if (inst->m_destroying) {
					flx::ui::GuiTask::unlock();
					return;
				}
				bool should_scan = flx::connectivity::ConnectivityManager::getInstance().isWiFiEnabled() && !inst->m_isScanning;
				if (should_scan) {
					inst->refreshScan();
				}
				flx::ui::GuiTask::unlock();
			},
			.arg = this,
			.dispatch_method = ESP_TIMER_TASK,
			.name = "wifi_scan_timer",
			.skip_unhandled_events = true,
		};
		esp_timer_create(&timer_args, &m_scanTimer);
		esp_timer_start_periodic(m_scanTimer, static_cast<uint64_t>(initial_interval) * 1000000);
	}
}

void WiFiSettings::onShow() {
	updateStatus();
	refreshScan();
}

void WiFiSettings::onHide() {
	if (m_connectContainer) {
		lv_obj_delete(m_connectContainer);
		m_connectContainer = nullptr;
		m_passwordTa = nullptr;
		m_saveSwitch = nullptr;
	}
	hideSavedNetworks();
	hideInfoPage();
	hideConfig();
}

void WiFiSettings::showConfig() {
	if (m_configContainer) return;

	if (m_container) {
		lv_obj_add_flag(m_container, LV_OBJ_FLAG_HIDDEN);
	}

	m_configContainer = create_page_container(m_parent);
	lv_obj_t* backBtn = nullptr;
	create_header(m_configContainer, "Wi-Fi Config", &backBtn);

	lv_obj_add_event_cb(
		backBtn,
		[](lv_event_t* e) {
			auto* instance = (WiFiSettings*)lv_event_get_user_data(e);
			instance->hideConfig();
		},
		LV_EVENT_CLICKED, this);

	lv_obj_t* list = create_settings_list(m_configContainer);

	// Saved Networks button
	lv_obj_t* savedBtn = lv_list_add_button(list, LV_SYMBOL_LIST, "Saved Networks");
	lv_obj_add_event_cb(
		savedBtn,
		[](lv_event_t* e) {
			auto* instance = (WiFiSettings*)lv_event_get_user_data(e);
			instance->showSavedNetworks();
		},
		LV_EVENT_CLICKED, this);

	// Auto scan switch
	lv_obj_t* toggleBtn = lv_list_add_button(list, nullptr, "Auto Scan");
	lv_obj_t* sw = lv_switch_create(toggleBtn);
	lv_obj_align(sw, LV_ALIGN_RIGHT_MID, 0, 0);
	lv_obj_add_flag(sw, LV_OBJ_FLAG_EVENT_BUBBLE);

	int32_t currentInterval = lv_subject_get_int(m_wifiScanIntervalBridge->getSubject());
	if (currentInterval > 0) {
		lv_obj_add_state(sw, LV_STATE_CHECKED);
		m_lastScanInterval = std::clamp(currentInterval, int32_t {10}, int32_t {120});
	} else if (m_lastScanInterval <= 0) {
		m_lastScanInterval = 10;
	}

	// Auto-scan interval slider (10-120 seconds)
	lv_obj_t* slider = add_slider_item(
		list,
		"Scan Interval (s)",
		m_wifiScanIntervalBridge->getSubject(),
		10, 120 // max 2 minutes
	);

	m_scanSliderRow = lv_obj_get_parent(slider);

	if (currentInterval <= 0) {
		lv_obj_add_flag(m_scanSliderRow, LV_OBJ_FLAG_HIDDEN);
	}

	lv_obj_add_event_cb(sw, [](lv_event_t* e) {
		auto* instance = (WiFiSettings*)lv_event_get_user_data(e);
		lv_obj_t* switch_obj = lv_event_get_target_obj(e);
		bool is_checked = lv_obj_has_state(switch_obj, LV_STATE_CHECKED);
		
		if (is_checked) {
			lv_obj_remove_flag(instance->m_scanSliderRow, LV_OBJ_FLAG_HIDDEN);
			lv_subject_set_int(instance->m_wifiScanIntervalBridge->getSubject(), instance->m_lastScanInterval);
		} else {
			lv_obj_add_flag(instance->m_scanSliderRow, LV_OBJ_FLAG_HIDDEN);
			int32_t cv = lv_subject_get_int(instance->m_wifiScanIntervalBridge->getSubject());
			if (cv > 0) {
				instance->m_lastScanInterval = cv;
			}
			lv_subject_set_int(instance->m_wifiScanIntervalBridge->getSubject(), 0);
		} }, LV_EVENT_VALUE_CHANGED, this);
}

void WiFiSettings::hideConfig() {
	if (m_configContainer) {
		lv_obj_delete(m_configContainer);
		m_configContainer = nullptr;
		m_scanSliderRow = nullptr;
		if (m_container) {
			lv_obj_remove_flag(m_container, LV_OBJ_FLAG_HIDDEN);
		}
	}
}

void WiFiSettings::refreshScan() {
	if (m_list == nullptr || m_isScanning) {
		return;
	}

	// Clear list
	lv_obj_clean(m_list);

	if (!flx::connectivity::ConnectivityManager::getInstance().isWiFiEnabled()) {
		lv_list_add_text(m_list, "Wi-Fi is disabled");
		return;
	}

	m_isScanning = true;
	lv_list_add_text(m_list, "Scanning...");

	flx::connectivity::WiFiManager::getInstance().scan(
		[this](std::vector<wifi_ap_record_t> networks) {
			flx::ui::GuiTask::lock();
			m_isScanning = false;
			if (m_list == nullptr) {
				flx::ui::GuiTask::unlock();
				return;
			}
			lv_obj_clean(m_list);

			if (networks.empty()) {
				lv_list_add_text(m_list, "No networks found");
				flx::ui::GuiTask::unlock();
				return;
			}

			// Sort by RSSI
			std::sort(networks.begin(), networks.end(), [](const wifi_ap_record_t& a, const wifi_ap_record_t& b) {
				return a.rssi > b.rssi;
			});

			m_scanResults = networks;

			std::vector<wifi_ap_record_t> savedNets;
			std::vector<wifi_ap_record_t> availableNets;
			auto& store = flx::connectivity::WiFiCredentialStore::getInstance();

			for (const auto& net : networks) {
				char ssid_buf[34];
				memcpy(ssid_buf, net.ssid, 33);
				ssid_buf[33] = '\0';
				if (store.contains(ssid_buf)) {
					savedNets.push_back(net);
				} else {
					availableNets.push_back(net);
				}
			}

			auto wifiClickCb = [](lv_event_t* e) {
				auto* instance = (WiFiSettings*)lv_event_get_user_data(e);
				auto* btn = (lv_obj_t*)lv_event_get_current_target(e);
				const char* ssid = lv_list_get_button_text(instance->m_list, btn);

				wifi_auth_mode_t authmode = WIFI_AUTH_OPEN;
				for (const auto& net: instance->m_scanResults) {
					if (strcmp((char*)net.ssid, ssid) == 0) {
						authmode = net.authmode;
						break;
					}
				}

				auto& store = flx::connectivity::WiFiCredentialStore::getInstance();
				flx::connectivity::WiFiCredential cred;
				bool is_saved = store.load(ssid, cred);

				if (is_saved) {
					flx::connectivity::ConnectivityManager::getInstance().connectWiFi(ssid, cred.password.c_str(), false);
					instance->updateStatus();
				} else if (authmode == WIFI_AUTH_OPEN) {
					flx::connectivity::ConnectivityManager::getInstance().connectWiFi(ssid, "");
					instance->updateStatus();
				} else {
					instance->showConnectScreen(ssid);
				}
			};

			auto populateList = [this, wifiClickCb](const std::vector<wifi_ap_record_t>& nets) {
				for (const auto& net: nets) {
					char ssid_buf[34];
					memcpy(ssid_buf, net.ssid, 33);
					ssid_buf[33] = '\0';

					const char* icon = getSignalIcon(net.rssi);
					lv_obj_t* btn = lv_list_add_button(m_list, icon, ssid_buf);
					lv_obj_add_event_cb(btn, wifiClickCb, LV_EVENT_CLICKED, this);

					lv_obj_t* rssi_label = lv_label_create(btn);
					lv_label_set_text_fmt(rssi_label, "%d dBm", net.rssi);
					lv_obj_set_style_text_opa(rssi_label, LV_OPA_60, 0);

					if (net.authmode != WIFI_AUTH_OPEN) {
						lv_obj_t* lock = lv_image_create(btn);
						lv_image_set_src(lock, LV_SYMBOL_EYE_CLOSE);
						lv_obj_align(lock, LV_ALIGN_RIGHT_MID, 0, 0);
					} else {
						lv_obj_align(rssi_label, LV_ALIGN_RIGHT_MID, 0, 0);
					}
				}
			};

			if (!savedNets.empty()) {
				lv_list_add_text(m_list, "Saved Networks");
				populateList(savedNets);
			}

			if (!availableNets.empty()) {
				lv_list_add_text(m_list, "Available Networks");
				populateList(availableNets);
			}
			flx::ui::GuiTask::unlock();
		});
}

const char* WiFiSettings::getSignalIcon(int8_t rssi) {
	if (rssi >= -50) {
		return LV_SYMBOL_WIFI;
	}
	if (rssi >= -70) {
		return LV_SYMBOL_WIFI; // Could use more granular icons if theme supports
	}
	return LV_SYMBOL_WIFI;
}

static const char* wifi_authmode_str(wifi_auth_mode_t mode) {
	switch (mode) {
		case WIFI_AUTH_OPEN: return "Open";
		case WIFI_AUTH_WEP: return "WEP";
		case WIFI_AUTH_WPA_PSK: return "WPA-PSK";
		case WIFI_AUTH_WPA2_PSK: return "WPA2-PSK";
		case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2-PSK";
		case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-Enterprise";
		case WIFI_AUTH_WPA3_PSK: return "WPA3-PSK";
		case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3-PSK";
		default: return "Unknown";
	}
}

static const char* wifi_phy_mode_str(wifi_ap_record_t const& ap_info) {
	if (ap_info.phy_11n) return "802.11n (Wi-Fi 4)";
	if (ap_info.phy_11g) return "802.11g";
	if (ap_info.phy_11b) return "802.11b";
	return "802.11";
}

void WiFiSettings::updateInfoPanel() {
	if (!m_infoContainer) return;

	wifi_ap_record_t ap_info;
	if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
		if (m_infoSsidLabel) {
			lv_label_set_text(m_infoSsidLabel, (char*)ap_info.ssid);
		}
		if (m_infoRssiLabel) {
			lv_label_set_text_fmt(m_infoRssiLabel, "%d dBm", ap_info.rssi);
		}
		if (m_infoChannelLabel) {
			lv_label_set_text_fmt(m_infoChannelLabel, "%d", ap_info.primary);
		}
		if (m_infoBssidLabel) {
			lv_label_set_text_fmt(m_infoBssidLabel, "%02x:%02x:%02x:%02x:%02x:%02x",
				ap_info.bssid[0], ap_info.bssid[1], ap_info.bssid[2],
				ap_info.bssid[3], ap_info.bssid[4], ap_info.bssid[5]);
		}
		if (m_infoSecurityLabel) {
			lv_label_set_text(m_infoSecurityLabel, wifi_authmode_str(ap_info.authmode));
		}
		if (m_infoStandardLabel) {
			lv_label_set_text(m_infoStandardLabel, wifi_phy_mode_str(ap_info));
		}
	}

	uint8_t mac[6];
	if (esp_wifi_get_mac(WIFI_IF_STA, mac) == ESP_OK) {
		if (m_infoMacLabel) {
			lv_label_set_text_fmt(m_infoMacLabel, "%02x:%02x:%02x:%02x:%02x:%02x",
				mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		}
	}

	esp_netif_t* sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
	if (sta_netif) {
		esp_netif_ip_info_t ip_info;
		if (esp_netif_get_ip_info(sta_netif, &ip_info) == ESP_OK) {
			char ip_str[16];
			char gw_str[16];
			char mask_str[16];
			esp_ip4addr_ntoa(&ip_info.ip, ip_str, sizeof(ip_str));
			esp_ip4addr_ntoa(&ip_info.gw, gw_str, sizeof(gw_str));
			esp_ip4addr_ntoa(&ip_info.netmask, mask_str, sizeof(mask_str));
			if (m_infoIpLabel) {
				lv_label_set_text(m_infoIpLabel, ip_str);
			}
			if (m_infoGatewayLabel) {
				lv_label_set_text(m_infoGatewayLabel, gw_str);
			}
			if (m_infoSubnetLabel) {
				lv_label_set_text(m_infoSubnetLabel, mask_str);
			}
		}
		esp_netif_dns_info_t dns_info;
		if (esp_netif_get_dns_info(sta_netif, ESP_NETIF_DNS_MAIN, &dns_info) == ESP_OK) {
			char dns_str[16];
			esp_ip4addr_ntoa(&dns_info.ip.u_addr.ip4, dns_str, sizeof(dns_str));
			if (m_infoDnsLabel) {
				lv_label_set_text(m_infoDnsLabel, dns_str);
			}
		}
	}
}

void WiFiSettings::updateStatus() {
	if (m_statusLabel == nullptr) {
		return;
	}

	auto const status = static_cast<flx::connectivity::WiFiStatus>(lv_subject_get_int(
		m_wifiStatusBridge->getSubject()));

	if (m_infoBtn) {
		if (status == flx::connectivity::WiFiStatus::CONNECTED) {
			lv_obj_remove_flag(m_infoBtn, LV_OBJ_FLAG_HIDDEN);
		} else {
			lv_obj_add_flag(m_infoBtn, LV_OBJ_FLAG_HIDDEN);
		}
	}

	if (status == flx::connectivity::WiFiStatus::CONNECTED) {
		if (m_infoContainer) {
			updateInfoPanel();
		}
	} else {
		hideInfoPage();
	}

	switch (status) {
		case flx::connectivity::WiFiStatus::RADIO_OFF:
			lv_label_set_text(m_statusLabel, "Disabled");
			break;
		case flx::connectivity::WiFiStatus::RADIO_ON_PENDING:
			lv_label_set_text(m_statusLabel, "Enabling...");
			break;
		case flx::connectivity::WiFiStatus::DISCONNECTED:
			lv_label_set_text(m_statusLabel, "Disconnected");
			break;
		case flx::connectivity::WiFiStatus::SCANNING:
			lv_label_set_text(m_statusLabel, "Scanning...");
			break;
		case flx::connectivity::WiFiStatus::CONNECTING:
			lv_label_set_text(m_statusLabel, "Connecting...");
			break;
		case flx::connectivity::WiFiStatus::CONNECTED: {
			wifi_ap_record_t ap_info;
			if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
				const char* auth_symbol = (ap_info.authmode == WIFI_AUTH_OPEN)
					? LV_SYMBOL_EYE_OPEN
					: LV_SYMBOL_EYE_CLOSE;
				lv_label_set_text_fmt(m_statusLabel, "Connected to %s %s (%d dBm)", (char*)ap_info.ssid, auth_symbol, ap_info.rssi);
			} else {
				lv_label_set_text(m_statusLabel, "Connected");
			}
			break;
		}
		case flx::connectivity::WiFiStatus::AUTH_FAILED:
			lv_label_set_text(m_statusLabel, "Authentication Failed");
			break;
		case flx::connectivity::WiFiStatus::NOT_FOUND:
			lv_label_set_text(m_statusLabel, "Network Not Found");
			break;
	}
}

void WiFiSettings::showConnectScreen(const char* ssid) {
	if (m_connectContainer != nullptr) {
		return;
	}

	if (m_container) {
		lv_obj_add_flag(m_container, LV_OBJ_FLAG_HIDDEN);
	}

	m_connectContainer = lv_obj_create(m_parent);
	lv_obj_set_size(m_connectContainer, lv_pct(100), lv_pct(100));
	lv_obj_set_style_pad_all(m_connectContainer, 0, 0);
	lv_obj_set_style_pad_row(m_connectContainer, 0, 0);
	lv_obj_set_style_border_width(m_connectContainer, 0, 0);
	lv_obj_set_flex_flow(m_connectContainer, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(m_connectContainer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_t* header = lv_obj_create(m_connectContainer);
	lv_obj_set_size(header, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_style_pad_all(header, lv_dpx(UiConstants::PAD_MEDIUM), 0);
	lv_obj_t* title = lv_label_create(header);
	lv_label_set_text_fmt(title, "Connect to %s", ssid);
	lv_obj_center(title);

	m_connectSsid = ssid;

	m_passwordTa = lv_textarea_create(m_connectContainer);
	lv_textarea_set_password_mode(m_passwordTa, true);
	lv_textarea_set_one_line(m_passwordTa, true);
	lv_textarea_set_placeholder_text(m_passwordTa, "Password");
	lv_obj_set_width(m_passwordTa, lv_pct(LayoutConstants::INPUT_WIDTH_PCT));
	lv_obj_align(m_passwordTa, LV_ALIGN_TOP_MID, 0, 0);

	lv_obj_t* btnCont = lv_obj_create(m_connectContainer);
	lv_obj_set_size(btnCont, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_style_pad_all(btnCont, lv_dpx(UiConstants::PAD_SMALL), 0);
	lv_obj_set_style_border_width(btnCont, 0, 0);
	lv_obj_set_style_bg_opa(btnCont, 0, 0);
	lv_obj_set_flex_flow(btnCont, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(btnCont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_gap(btnCont, lv_dpx(UiConstants::PAD_MEDIUM), 0);
	lv_obj_remove_flag(btnCont, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t* cancelBtn = lv_button_create(btnCont);
	lv_obj_t* cancelLabel = lv_label_create(cancelBtn);
	lv_label_set_text(cancelLabel, "Cancel");
	lv_obj_add_event_cb(
		cancelBtn,
		[](lv_event_t* e) {
			auto* instance = (WiFiSettings*)lv_event_get_user_data(e);
			lv_obj_delete(instance->m_connectContainer);
			instance->m_connectContainer = nullptr;
			instance->m_passwordTa = nullptr;
			instance->m_saveSwitch = nullptr;
			if (instance->m_container) {
				lv_obj_remove_flag(instance->m_container, LV_OBJ_FLAG_HIDDEN);
			}
		},
		LV_EVENT_CLICKED, this);

	lv_obj_t* connectBtn = lv_button_create(btnCont);
	lv_obj_t* connectLabel = lv_label_create(connectBtn);
	lv_label_set_text(connectLabel, "Connect");
	lv_obj_add_event_cb(
		connectBtn,
		[](lv_event_t* e) {
			auto* instance = (WiFiSettings*)lv_event_get_user_data(e);
			const char* password = lv_textarea_get_text(instance->m_passwordTa);
			bool save = lv_obj_has_state(instance->m_saveSwitch, LV_STATE_CHECKED);
			flx::connectivity::ConnectivityManager::getInstance().connectWiFi(
				instance->m_connectSsid.c_str(), password, save);
			lv_obj_delete(instance->m_connectContainer);
			instance->m_connectContainer = nullptr;
			instance->m_passwordTa = nullptr;
			instance->m_saveSwitch = nullptr;
			instance->updateStatus();
			if (instance->m_container) {
				lv_obj_remove_flag(instance->m_container, LV_OBJ_FLAG_HIDDEN);
			}
		},
		LV_EVENT_CLICKED, this);

	// Save switch container
	lv_obj_t* save_cont = lv_obj_create(m_connectContainer);
	lv_obj_set_size(save_cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_style_pad_all(save_cont, 0, 0);
	lv_obj_set_style_border_width(save_cont, 0, 0);
	lv_obj_set_flex_flow(save_cont, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(save_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_gap(save_cont, lv_dpx(UiConstants::PAD_SMALL), 0);

	lv_obj_t* save_label = lv_label_create(save_cont);
	lv_label_set_text(save_label, "Save");

	m_saveSwitch = lv_switch_create(save_cont);
	lv_obj_add_state(m_saveSwitch, LV_STATE_CHECKED); // Default to save

	lv_obj_add_state(m_passwordTa, LV_STATE_FOCUSED);
}

void WiFiSettings::onDestroy() {
	m_destroying = true;
	if (m_scanTimer != nullptr) {
		esp_timer_stop(m_scanTimer);
		esp_timer_delete(m_scanTimer);
		m_scanTimer = nullptr;
	}
	if (m_connectContainer) {
		lv_obj_delete(m_connectContainer);
		m_connectContainer = nullptr;
	}
	if (m_savedNetContainer) {
		lv_obj_delete(m_savedNetContainer);
		m_savedNetContainer = nullptr;
		m_savedNetList = nullptr;
	}
	hideInfoPage();
	hideConfig();
	// m_container is deleted by base class
	m_list = nullptr;
	m_wifiSwitch = nullptr;
	m_statusLabel = nullptr;
	m_statusPrefixLabel = nullptr;
	m_saveSwitch = nullptr;
	m_scanSliderRow = nullptr;
	m_statusObserver = nullptr; // Auto-cleaned when m_container is deleted
	m_scanIntervalObserver = nullptr; // Auto-cleaned when m_container is deleted
	m_infoContainer = nullptr;
	m_infoBtn = nullptr;
	m_infoSsidLabel = nullptr;
	m_infoIpLabel = nullptr;
	m_infoSubnetLabel = nullptr;
	m_infoGatewayLabel = nullptr;
	m_infoDnsLabel = nullptr;
	m_infoSecurityLabel = nullptr;
	m_infoStandardLabel = nullptr;
	m_infoRssiLabel = nullptr;
	m_infoChannelLabel = nullptr;
	m_infoBssidLabel = nullptr;
	m_infoMacLabel = nullptr;
}

// ──────────────────────────────────────────────────────────────────────
// Saved Networks sub-page
// ──────────────────────────────────────────────────────────────────────

void WiFiSettings::showSavedNetworks() {
	if (m_savedNetContainer) return;

	if (m_configContainer) {
		lv_obj_add_flag(m_configContainer, LV_OBJ_FLAG_HIDDEN);
	}

	m_savedNetContainer = create_page_container(m_parent);
	lv_obj_t* backBtn = nullptr;
	create_header(m_savedNetContainer, "Saved Networks", &backBtn);

	lv_obj_add_event_cb(
		backBtn,
		[](lv_event_t* e) {
			auto* instance = (WiFiSettings*)lv_event_get_user_data(e);
			instance->hideSavedNetworks();
		},
		LV_EVENT_CLICKED, this);

	m_savedNetList = create_settings_list(m_savedNetContainer);
	refreshSavedNetworksList();
}

void WiFiSettings::hideSavedNetworks() {
	if (m_savedNetContainer) {
		lv_obj_delete(m_savedNetContainer);
		m_savedNetContainer = nullptr;
		m_savedNetList = nullptr;
		if (m_configContainer) {
			lv_obj_remove_flag(m_configContainer, LV_OBJ_FLAG_HIDDEN);
		}
	}
}

void WiFiSettings::refreshSavedNetworksList() {
	if (!m_savedNetList) return;
	lv_obj_clean(m_savedNetList);

	auto networks = flx::connectivity::ConnectivityManager::getInstance().getSavedNetworks();

	if (networks.empty()) {
		lv_list_add_text(m_savedNetList, "No saved networks");
		return;
	}

	for (const auto& net: networks) {
		// Row: SSID label + priority badge
		lv_obj_t* btn = lv_list_add_button(m_savedNetList, LV_SYMBOL_WIFI, net.ssid.c_str());

		// Priority badge
		lv_obj_t* priBadge = lv_label_create(btn);
		char priBuf[16];
		snprintf(priBuf, sizeof(priBuf), "P%d", net.priority);
		lv_label_set_text(priBadge, priBuf);
		lv_obj_set_style_text_opa(priBadge, LV_OPA_60, 0);

		// Auto-connect toggle
		lv_obj_t* acSwitch = lv_switch_create(btn);
		if (net.autoConnect) {
			lv_obj_add_state(acSwitch, LV_STATE_CHECKED);
		}
		// Capture ssid by value; heap-allocate for LVGL user data lifetime
		std::string ssid_copy = net.ssid;
		auto* ssid_ud = new std::string(ssid_copy);
		lv_obj_add_event_cb(
			acSwitch,
			[](lv_event_t* e) {
				auto* sw = lv_event_get_target_obj(e);
				auto* ssid_ptr = static_cast<std::string*>(lv_event_get_user_data(e));
				bool ac = lv_obj_has_state(sw, LV_STATE_CHECKED);
				flx::connectivity::WiFiCredential cred;
				if (flx::connectivity::WiFiCredentialStore::getInstance().load(*ssid_ptr, cred)) {
					cred.autoConnect = ac;
					flx::connectivity::WiFiCredentialStore::getInstance().save(cred);
				}
			},
			LV_EVENT_VALUE_CHANGED, ssid_ud);

		// Free the heap-allocated SSID string when the switch is destroyed
		lv_obj_add_event_cb(
			acSwitch,
			[](lv_event_t* e) {
				delete static_cast<std::string*>(lv_event_get_user_data(e));
			},
			LV_EVENT_DELETE, ssid_ud);

		// Forget button
		lv_obj_t* forgetBtn = lv_button_create(btn);
		lv_obj_set_size(forgetBtn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		lv_obj_t* forgetLabel = lv_label_create(forgetBtn);
		lv_label_set_text(forgetLabel, LV_SYMBOL_TRASH);

		lv_obj_add_event_cb(
			forgetBtn,
			[](lv_event_t* e) {
				auto* instance = (WiFiSettings*)lv_event_get_user_data(e);
				auto* btn_obj = lv_obj_get_parent(lv_event_get_target_obj(e));
				const char* ssid = lv_list_get_button_text(instance->m_savedNetList, btn_obj);
				if (ssid) {
					flx::connectivity::ConnectivityManager::getInstance().removeWiFiNetwork(std::string(ssid));
					instance->refreshSavedNetworksList();
				}
			},
			LV_EVENT_CLICKED, this);
	}
}

void WiFiSettings::showInfoPage() {
	if (m_infoContainer) return;

	if (m_container) {
		lv_obj_add_flag(m_container, LV_OBJ_FLAG_HIDDEN);
	}

	m_infoContainer = create_page_container(m_parent);
	lv_obj_t* backBtn = nullptr;
	lv_obj_t* header = create_header(m_infoContainer, "Wi-Fi Info", &backBtn);

	lv_obj_add_event_cb(
		backBtn,
		[](lv_event_t* e) {
			auto* instance = (WiFiSettings*)lv_event_get_user_data(e);
			instance->hideInfoPage();
		},
		LV_EVENT_CLICKED, this);

	// Forget Network button beside the wifi info in header, right aligned
	lv_obj_t* forgetBtn = lv_button_create(header);
	lv_obj_set_size(forgetBtn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_add_flag(forgetBtn, LV_OBJ_FLAG_FLOATING);
	lv_obj_align(forgetBtn, LV_ALIGN_RIGHT_MID, -lv_dpx(UiConstants::PAD_SMALL), 0);
	lv_obj_set_style_pad_all(forgetBtn, lv_dpx(4), 0);

	lv_obj_t* forgetLabel = lv_label_create(forgetBtn);
	lv_label_set_text(forgetLabel, "Forget");

	lv_obj_add_event_cb(forgetBtn, [](lv_event_t* e) {
		auto* instance = (WiFiSettings*)lv_event_get_user_data(e);
		auto& cm = flx::connectivity::ConnectivityManager::getInstance();
		wifi_ap_record_t ap_info;
		if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
			cm.removeWiFiNetwork(std::string((char*)ap_info.ssid));
			cm.disconnectWiFi();
		}
		instance->hideInfoPage();
	}, LV_EVENT_CLICKED, this);

	lv_obj_t* list = create_settings_list(m_infoContainer);

	auto addInfoRow = [](lv_obj_t* parent, const char* labelStr, lv_obj_t** valLabelOut) {
		lv_obj_t* row = lv_list_add_button(parent, nullptr, labelStr);
		*valLabelOut = lv_label_create(row);
		lv_obj_align(*valLabelOut, LV_ALIGN_RIGHT_MID, 0, 0);
		lv_label_set_text(*valLabelOut, "-");
	};

	addInfoRow(list, "SSID", &m_infoSsidLabel);
	addInfoRow(list, "IP Address", &m_infoIpLabel);
	addInfoRow(list, "Subnet Mask", &m_infoSubnetLabel);
	addInfoRow(list, "Gateway", &m_infoGatewayLabel);
	addInfoRow(list, "DNS Server", &m_infoDnsLabel);
	addInfoRow(list, "Security", &m_infoSecurityLabel);
	addInfoRow(list, "Wi-Fi Standard", &m_infoStandardLabel);
	addInfoRow(list, "RSSI", &m_infoRssiLabel);
	addInfoRow(list, "Channel", &m_infoChannelLabel);
	addInfoRow(list, "BSSID", &m_infoBssidLabel);
	addInfoRow(list, "Device MAC", &m_infoMacLabel);

	updateInfoPanel();
}

void WiFiSettings::hideInfoPage() {
	if (m_infoContainer) {
		lv_obj_delete(m_infoContainer);
		m_infoContainer = nullptr;
		m_infoSsidLabel = nullptr;
		m_infoIpLabel = nullptr;
		m_infoSubnetLabel = nullptr;
		m_infoGatewayLabel = nullptr;
		m_infoDnsLabel = nullptr;
		m_infoSecurityLabel = nullptr;
		m_infoStandardLabel = nullptr;
		m_infoRssiLabel = nullptr;
		m_infoChannelLabel = nullptr;
		m_infoBssidLabel = nullptr;
		m_infoMacLabel = nullptr;
		if (m_container) {
			lv_obj_remove_flag(m_container, LV_OBJ_FLAG_HIDDEN);
		}
	}
}

} // namespace System::Apps::Settings
