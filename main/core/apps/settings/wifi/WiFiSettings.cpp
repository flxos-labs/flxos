#include "WiFiSettings.hpp"
#include "core/apps/settings/SettingsCommon.hpp"
#include "core/connectivity/ConnectivityManager.hpp"
#include "core/connectivity/wifi/WiFiManager.hpp"
#include "core/lv_obj.h"
#include "core/lv_obj_event.h"
#include "core/lv_obj_pos.h"
#include "core/lv_obj_style.h"
#include "core/lv_obj_style_gen.h"
#include "core/lv_obj_tree.h"
#include "core/lv_observer.h"
#include "core/tasks/gui/GuiTask.hpp"
#include "core/ui/theming/layout_constants/LayoutConstants.hpp"
#include "core/ui/theming/ui_constants/UiConstants.hpp"
#include "display/lv_display.h"
#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_wifi_types_generic.h"
#include "font/lv_symbol_def.h"
#include "layouts/flex/lv_flex.h"
#include "misc/lv_area.h"
#include "misc/lv_event.h"
#include "misc/lv_types.h"
#include "widgets/button/lv_button.h"
#include "widgets/image/lv_image.h"
#include "widgets/label/lv_label.h"
#include "widgets/list/lv_list.h"
#include "widgets/switch/lv_switch.h"
#include "widgets/textarea/lv_textarea.h"
#include <algorithm>
#include <cstdint>

namespace System::Apps::Settings {

// Constructor removed (using inherited)

void WiFiSettings::createUI() {
	m_container = create_page_container(m_parent);
	lv_obj_set_style_pad_gap(m_container, 0, 0);

	lv_obj_t* backBtn = nullptr;
	lv_obj_t* header = create_header(m_container, "Wi-Fi", &backBtn);
	add_back_button_event_cb(backBtn, &m_onBack);

	lv_obj_t* title = lv_obj_get_child(header, 1);
	lv_obj_set_flex_grow(title, 1);

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
		LV_EVENT_CLICKED, this
	);

	m_wifiSwitch = lv_switch_create(header);
	lv_obj_bind_checked(
		m_wifiSwitch,
		&ConnectivityManager::getInstance().getWiFiEnabledSubject()
	);

	lv_obj_add_event_cb(
		m_wifiSwitch,
		[](lv_event_t* e) {
			auto* sw = lv_event_get_target_obj(e);
			auto* instance = (WiFiSettings*)lv_event_get_user_data(e);
			bool const enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
			ConnectivityManager::getInstance().setWiFiEnabled(enabled);
			if (!enabled) {
				instance->m_isScanning = false;
				lv_obj_clean(instance->m_list);
				lv_list_add_text(instance->m_list, "Wi-Fi is disabled");
			} else {
				// Auto-connect to saved network if available
				auto& cm = ConnectivityManager::getInstance();
				if (cm.hasSavedWiFiCredentials()) {
					cm.connectWiFi(
						cm.getSavedWiFiSsid(),
						cm.getSavedWiFiPassword(),
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
		LV_EVENT_VALUE_CHANGED, this
	);

	lv_obj_t* statusCont = lv_obj_create(m_container);
	lv_obj_set_size(statusCont, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_style_pad_all(statusCont, 0, 0);
	lv_obj_set_style_pad_gap(statusCont, 0, 0);
	lv_obj_set_style_pad_hor(statusCont, lv_dpx(UiConstants::PAD_MEDIUM), 0);
	lv_obj_set_style_border_width(statusCont, 0, 0);
	lv_obj_set_flex_flow(statusCont, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(statusCont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	m_statusPrefixLabel = lv_label_create(statusCont);
	lv_label_set_text(m_statusPrefixLabel, "Status: ");

	m_statusLabel = lv_label_create(statusCont);
	lv_obj_set_flex_grow(m_statusLabel, 1);
	lv_label_set_long_mode(m_statusLabel, LV_LABEL_LONG_MODE_SCROLL);
	updateStatus();

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
		LV_EVENT_CLICKED, this
	);

	m_list = create_settings_list(m_container);

	// Observer for WiFi status changes - triggers auto-scan after connection
	m_statusObserver = lv_subject_add_observer_obj(
		&ConnectivityManager::getInstance().getWiFiConnectedSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			auto* instance = (WiFiSettings*)lv_observer_get_user_data(observer);
			int32_t connected = lv_subject_get_int(subject);
			if (connected && instance->m_pendingAutoScan) {
				instance->m_pendingAutoScan = false;
				instance->refreshScan();
			}
			instance->updateStatus();
		},
		m_container, this
	);

	// Observer for scan interval setting changes
	m_scanIntervalObserver = lv_subject_add_observer_obj(
		&ConnectivityManager::getInstance().getWiFiScanIntervalSubject(),
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
						GuiTask::lock();
						bool should_scan = ConnectivityManager::getInstance().isWiFiEnabled() && !inst->m_isScanning;
						GuiTask::unlock();
						if (should_scan) {
							GuiTask::lock();
							inst->refreshScan();
							GuiTask::unlock();
						}
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
		m_container, this
	);

	// Trigger initial timer setup based on current setting
	int32_t initial_interval = lv_subject_get_int(&ConnectivityManager::getInstance().getWiFiScanIntervalSubject());
	if (initial_interval > 0) {
		esp_timer_create_args_t timer_args = {
			.callback = [](void* arg) {
				auto* inst = static_cast<WiFiSettings*>(arg);
				GuiTask::lock();
				bool should_scan = ConnectivityManager::getInstance().isWiFiEnabled() && !inst->m_isScanning;
				GuiTask::unlock();
				if (should_scan) {
					GuiTask::lock();
					inst->refreshScan();
					GuiTask::unlock();
				}
			},
			.arg = this,
			.dispatch_method = ESP_TIMER_TASK,
			.name = "wifi_scan_timer",
			.skip_unhandled_events = true,
		};
		esp_timer_create(&timer_args, &m_scanTimer);
		esp_timer_start_periodic(m_scanTimer, static_cast<uint64_t>(initial_interval) * 1000000);
	}

	refreshScan();
}

void WiFiSettings::onShow() {
	updateStatus();
	refreshScan();
}

void WiFiSettings::showConfig() {
	if (m_configContainer) return;

	m_configContainer = create_page_container(m_parent);
	lv_obj_t* backBtn = nullptr;
	create_header(m_configContainer, "Wi-Fi Config", &backBtn);

	lv_obj_add_event_cb(
		backBtn,
		[](lv_event_t* e) {
			auto* instance = (WiFiSettings*)lv_event_get_user_data(e);
			instance->hideConfig();
		},
		LV_EVENT_CLICKED, this
	);

	lv_obj_t* list = create_settings_list(m_configContainer);

	add_switch_item(
		list,
		"Auto-start on Boot",
		&ConnectivityManager::getInstance().getWiFiAutostartSubject()
	);

	// Auto-scan interval slider (0 = disabled, 10-120 seconds)
	add_slider_item(
		list,
		"Scan Interval (s)",
		&ConnectivityManager::getInstance().getWiFiScanIntervalSubject(),
		0, 120 // 0 = disabled, max 2 minutes
	);
}

void WiFiSettings::hideConfig() {
	if (m_configContainer) {
		lv_obj_delete(m_configContainer);
		m_configContainer = nullptr;
	}
}

void WiFiSettings::refreshScan() {
	if (m_list == nullptr || m_isScanning) {
		return;
	}

	// Clear list
	lv_obj_clean(m_list);

	if (!ConnectivityManager::getInstance().isWiFiEnabled()) {
		lv_list_add_text(m_list, "Wi-Fi is disabled");
		return;
	}

	m_isScanning = true;
	lv_list_add_text(m_list, "Scanning...");

	ConnectivityManager::getInstance().scanWiFi(
		[this](std::vector<wifi_ap_record_t> networks) {
			GuiTask::lock();
			m_isScanning = false;
			if (m_list == nullptr) {
				GuiTask::unlock();
				return;
			}
			lv_obj_clean(m_list);

			if (networks.empty()) {
				lv_list_add_text(m_list, "No networks found");
				GuiTask::unlock();
				return;
			}

			// Sort by RSSI
			std::sort(networks.begin(), networks.end(), [](const wifi_ap_record_t& a, const wifi_ap_record_t& b) {
				return a.rssi > b.rssi;
			});

			m_scanResults = networks;

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

				if (authmode == WIFI_AUTH_OPEN) {
					ConnectivityManager::getInstance().connectWiFi(ssid, "");
					instance->updateStatus();
				} else {
					instance->showConnectScreen(ssid);
				}
			};

			for (const auto& net: networks) {
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
			GuiTask::unlock();
		}
	);
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

void WiFiSettings::updateStatus() {
	if (m_statusLabel == nullptr) {
		return;
	}

	auto const status = static_cast<System::WiFiStatus>(lv_subject_get_int(
		&ConnectivityManager::getInstance().getWiFiStatusSubject()
	));

	switch (status) {
		case System::WiFiStatus::DISABLED:
			lv_label_set_text(m_statusLabel, "Disabled");
			break;
		case System::WiFiStatus::DISCONNECTED:
			lv_label_set_text(m_statusLabel, "Disconnected");
			break;
		case System::WiFiStatus::SCANNING:
			break;
		case System::WiFiStatus::CONNECTING:
			lv_label_set_text(m_statusLabel, "Connecting...");
			break;
		case System::WiFiStatus::CONNECTED: {
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
		case System::WiFiStatus::AUTH_FAILED:
			lv_label_set_text(m_statusLabel, "Authentication Failed");
			break;
		case System::WiFiStatus::NOT_FOUND:
			lv_label_set_text(m_statusLabel, "Network Not Found");
			break;
	}
}

void WiFiSettings::showConnectScreen(const char* ssid) {
	if (m_connectContainer != nullptr) {
		return;
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
		},
		LV_EVENT_CLICKED, this
	);

	lv_obj_t* connectBtn = lv_button_create(btnCont);
	lv_obj_t* connectLabel = lv_label_create(connectBtn);
	lv_label_set_text(connectLabel, "Connect");
	lv_obj_add_event_cb(
		connectBtn,
		[](lv_event_t* e) {
			auto* instance = (WiFiSettings*)lv_event_get_user_data(e);
			const char* password = lv_textarea_get_text(instance->m_passwordTa);
			bool save = lv_obj_has_state(instance->m_saveSwitch, LV_STATE_CHECKED);
			ConnectivityManager::getInstance().connectWiFi(
				instance->m_connectSsid.c_str(), password, save
			);
			lv_obj_delete(instance->m_connectContainer);
			instance->m_connectContainer = nullptr;
			instance->m_passwordTa = nullptr;
			instance->m_saveSwitch = nullptr;
			instance->updateStatus();
		},
		LV_EVENT_CLICKED, this
	);

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
	if (m_scanTimer != nullptr) {
		esp_timer_stop(m_scanTimer);
		esp_timer_delete(m_scanTimer);
		m_scanTimer = nullptr;
	}
	if (m_connectContainer) {
		lv_obj_delete(m_connectContainer);
		m_connectContainer = nullptr;
	}
	hideConfig();
	// m_container is deleted by base class
	m_list = nullptr;
	m_wifiSwitch = nullptr;
	m_statusLabel = nullptr;
	m_statusPrefixLabel = nullptr;
	m_saveSwitch = nullptr;
	m_statusObserver = nullptr; // Auto-cleaned when m_container is deleted
	m_scanIntervalObserver = nullptr; // Auto-cleaned when m_container is deleted
}

} // namespace System::Apps::Settings
