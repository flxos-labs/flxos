#pragma once

#include "core/apps/settings/SettingsCommon.hpp"
#include "core/connectivity/ConnectivityManager.hpp"
#include "core/tasks/gui/GuiTask.hpp"
#include "esp_wifi.h"
#include "lvgl.h"
#include <algorithm>
#include <functional>
#include <string>

namespace System {
namespace Apps {
namespace Settings {

class WiFiSettings {
public:

	WiFiSettings(lv_obj_t* parent, std::function<void()> onBack)
		: m_parent(parent), m_onBack(onBack) {}

	void show() {
		if (m_container == nullptr) {
			m_container = create_page_container(m_parent);
			lv_obj_set_style_pad_gap(m_container, 0, 0);

			lv_obj_t* backBtn;
			lv_obj_t* header = create_header(m_container, "Wi-Fi", &backBtn);
			add_back_button_event_cb(backBtn, &m_onBack);

			lv_obj_t* title = lv_obj_get_child(header, 1);
			lv_obj_set_flex_grow(title, 1);

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
					bool enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
					ConnectivityManager::getInstance().setWiFiEnabled(enabled);
					if (!enabled) {
						instance->m_isScanning = false;
						lv_obj_clean(instance->m_list);
						lv_list_add_text(instance->m_list, "Wi-Fi is disabled");
					} else {
						instance->refreshScan();
					}
					instance->updateStatus();
				},
				LV_EVENT_VALUE_CHANGED, this
			);

			lv_obj_t* statusCont = lv_obj_create(m_container);
			lv_obj_set_size(statusCont, lv_pct(100), LV_SIZE_CONTENT);
			lv_obj_set_style_pad_all(statusCont, 0, 0);
			lv_obj_set_style_pad_gap(statusCont, 0, 0);
			lv_obj_set_style_pad_hor(statusCont, lv_dpx(5), 0);
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

			m_timer = lv_timer_create(
				[](lv_timer_t* t) {
					auto* instance = (WiFiSettings*)lv_timer_get_user_data(t);
					instance->updateStatus();
				},
				1000, this
			);

			refreshScan();
		} else {
			lv_obj_remove_flag(m_container, LV_OBJ_FLAG_HIDDEN);
			if (m_timer)
				lv_timer_resume(m_timer);
			updateStatus();
			refreshScan();
		}
	}

	void refreshScan() {
		if (m_list == nullptr || m_isScanning)
			return;

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
					lv_obj_t* btn = (lv_obj_t*)lv_event_get_current_target(e);
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

	const char* getSignalIcon(int8_t rssi) {
		if (rssi >= -50)
			return LV_SYMBOL_WIFI;
		if (rssi >= -70)
			return LV_SYMBOL_WIFI; // Could use more granular icons if theme supports
		return LV_SYMBOL_WIFI;
	}

	void updateStatus() {
		if (m_statusLabel == nullptr)
			return;

		WiFiStatus status = static_cast<WiFiStatus>(lv_subject_get_int(
			&ConnectivityManager::getInstance().getWiFiStatusSubject()
		));

		switch (status) {
			case WiFiStatus::DISABLED:
				lv_label_set_text(m_statusLabel, "Disabled");
				break;
			case WiFiStatus::DISCONNECTED:
				lv_label_set_text(m_statusLabel, "Disconnected");
				break;
			case WiFiStatus::SCANNING:
				break;
			case WiFiStatus::CONNECTING:
				lv_label_set_text(m_statusLabel, "Connecting...");
				break;
			case WiFiStatus::CONNECTED: {
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
			case WiFiStatus::AUTH_FAILED:
				lv_label_set_text(m_statusLabel, "Authentication Failed");
				break;
			case WiFiStatus::NOT_FOUND:
				lv_label_set_text(m_statusLabel, "Network Not Found");
				break;
		}
	}

	void showConnectScreen(const char* ssid) {
		if (m_connectContainer != nullptr)
			return;

		m_connectContainer = lv_obj_create(m_parent);
		lv_obj_set_size(m_connectContainer, lv_pct(100), lv_pct(100));
		lv_obj_set_style_pad_all(m_connectContainer, 0, 0);
		lv_obj_set_style_pad_row(m_connectContainer, 0, 0);
		lv_obj_set_style_border_width(m_connectContainer, 0, 0);
		lv_obj_set_style_bg_color(m_connectContainer, lv_palette_main(LV_PALETTE_GREY), 0);
		lv_obj_set_flex_flow(m_connectContainer, LV_FLEX_FLOW_COLUMN);
		lv_obj_set_flex_align(m_connectContainer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		lv_obj_t* header = lv_obj_create(m_connectContainer);
		lv_obj_set_size(header, lv_pct(100), LV_SIZE_CONTENT);
		lv_obj_set_style_pad_all(header, lv_dpx(5), 0);
		lv_obj_t* title = lv_label_create(header);
		lv_label_set_text_fmt(title, "Connect to %s", ssid);
		lv_obj_center(title);

		m_connectSsid = ssid;

		m_passwordTa = lv_textarea_create(m_connectContainer);
		lv_textarea_set_password_mode(m_passwordTa, true);
		lv_textarea_set_one_line(m_passwordTa, true);
		lv_textarea_set_placeholder_text(m_passwordTa, "Password");
		lv_obj_set_width(m_passwordTa, lv_pct(90));
		lv_obj_align(m_passwordTa, LV_ALIGN_TOP_MID, 0, 0);

		lv_obj_t* btnCont = lv_obj_create(m_connectContainer);
		lv_obj_set_size(btnCont, lv_pct(100), LV_SIZE_CONTENT);
		lv_obj_set_style_pad_all(btnCont, 0, 0);
		lv_obj_set_style_border_width(btnCont, 0, 0);
		lv_obj_set_style_bg_opa(btnCont, 0, 0);
		lv_obj_set_flex_flow(btnCont, LV_FLEX_FLOW_ROW);
		lv_obj_set_flex_align(btnCont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

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
				ConnectivityManager::getInstance().connectWiFi(
					instance->m_connectSsid.c_str(), password
				);
				lv_obj_delete(instance->m_connectContainer);
				instance->m_connectContainer = nullptr;
				instance->m_passwordTa = nullptr;
				instance->updateStatus();
			},
			LV_EVENT_CLICKED, this
		);

		lv_obj_add_state(m_passwordTa, LV_STATE_FOCUSED);
	}

	void hide() {
		if (m_container) {
			lv_obj_add_flag(m_container, LV_OBJ_FLAG_HIDDEN);
		}
		if (m_timer) {
			lv_timer_pause(m_timer);
		}
		if (m_connectContainer) {
			lv_obj_delete(m_connectContainer);
			m_connectContainer = nullptr;
		}
	}

	void destroy() {
		if (m_timer) {
			lv_timer_delete(m_timer);
			m_timer = nullptr;
		}
		if (m_connectContainer) {
			lv_obj_delete(m_connectContainer);
			m_connectContainer = nullptr;
		}
		m_container = nullptr;
		m_list = nullptr;
		m_wifiSwitch = nullptr;
		m_statusLabel = nullptr;
		m_statusPrefixLabel = nullptr;
	}

private:

	lv_obj_t* m_parent;
	lv_obj_t* m_container = nullptr;
	lv_obj_t* m_connectContainer = nullptr;
	lv_obj_t* m_list = nullptr;
	lv_obj_t* m_wifiSwitch = nullptr;
	lv_obj_t* m_statusLabel = nullptr;
	lv_obj_t* m_statusPrefixLabel = nullptr;
	lv_obj_t* m_passwordTa = nullptr;
	std::string m_connectSsid;
	lv_timer_t* m_timer = nullptr;
	bool m_isScanning = false;
	std::function<void()> m_onBack;
	std::vector<wifi_ap_record_t> m_scanResults;
};

} // namespace Settings
} // namespace Apps
} // namespace System
