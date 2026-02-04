#pragma once

#include "core/apps/settings/SettingsCommon.hpp"
#include "core/connectivity/ConnectivityManager.hpp"
#include "core/connectivity/bluetooth/BluetoothManager.hpp"
#include "core/ui/theming/ui_constants/UiConstants.hpp"
#include "lvgl.h"
#include <functional>

namespace System::Apps::Settings {

class BluetoothSettings {
public:

	BluetoothSettings(lv_obj_t* parent, std::function<void()> onBack)
		: m_parent(parent), m_onBack(onBack) {}

	void show() {
		if (m_container == nullptr) {
			m_container = create_page_container(m_parent);
			lv_obj_set_style_pad_gap(m_container, 0, 0);

			lv_obj_t* backBtn = nullptr;
			lv_obj_t* header = create_header(m_container, "Bluetooth", &backBtn);
			add_back_button_event_cb(backBtn, &m_onBack);

			lv_obj_t* title = lv_obj_get_child(header, 1);
			lv_obj_set_flex_grow(title, 1);

			m_btSwitch = lv_switch_create(header);
			lv_obj_bind_checked(
				m_btSwitch,
				&ConnectivityManager::getInstance().getBluetoothEnabledSubject()
			);

			lv_obj_add_event_cb(
				m_btSwitch,
				[](lv_event_t* e) {
					auto* sw = lv_event_get_target_obj(e);
					auto* instance = (BluetoothSettings*)lv_event_get_user_data(e);
					bool const enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
					BluetoothManager::getInstance().enable(enabled);
					instance->refresh();
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

			lv_obj_t* statusPrefix = lv_label_create(statusCont);
			lv_label_set_text(statusPrefix, "Status: ");

			m_statusLabel = lv_label_create(statusCont);
			lv_obj_set_flex_grow(m_statusLabel, 1);
			lv_label_set_text(
				m_statusLabel,
				BluetoothManager::getInstance().isEnabled() ? "Ready" : "Disabled"
			);

			lv_obj_t* refreshBtn = lv_button_create(statusCont);
			lv_obj_set_size(refreshBtn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
			lv_obj_t* refreshLabel = lv_image_create(refreshBtn);
			lv_image_set_src(refreshLabel, LV_SYMBOL_REFRESH);
			lv_obj_add_event_cb(
				refreshBtn,
				[](lv_event_t* e) {
					auto* instance = (BluetoothSettings*)lv_event_get_user_data(e);
					instance->startScan();
				},
				LV_EVENT_CLICKED, this
			);

			m_list = create_settings_list(m_container);
			refresh();
		} else {
			lv_obj_remove_flag(m_container, LV_OBJ_FLAG_HIDDEN);
			refresh();
		}
	}

	void startScan() {
		if (!BluetoothManager::getInstance().isEnabled()) {
			return;
		}
		lv_obj_clean(m_list);
		lv_list_add_text(m_list, "Scanning for devices...");
		lv_label_set_text(m_statusLabel, "Scanning...");

		// Mock scan result after 2 seconds
		lv_timer_create(
			[](lv_timer_t* t) {
				auto* instance = (BluetoothSettings*)lv_timer_get_user_data(t);
				if (instance && instance->m_list) {
					lv_obj_clean(instance->m_list);
					lv_list_add_text(instance->m_list, "Available Devices");
					lv_list_add_button(instance->m_list, LV_SYMBOL_AUDIO, "Wireless Headphones");
					lv_list_add_button(instance->m_list, LV_SYMBOL_KEYBOARD, "Mechanical Keyboard");
					lv_list_add_button(instance->m_list, LV_SYMBOL_BLUETOOTH, "Smartphone");
					lv_label_set_text(instance->m_statusLabel, "Scan Complete");
				}
				lv_timer_delete(t); // One-shot
			},
			2000, this
		);
	}

	void refresh() {
		if (m_list == nullptr) {
			return;
		}
		lv_obj_clean(m_list);

		bool const enabled = BluetoothManager::getInstance().isEnabled();
		lv_label_set_text(m_statusLabel, enabled ? "Ready" : "Disabled");

		if (enabled) {
			lv_list_add_text(m_list, "Paired Devices");
			lv_list_add_text(m_list, "No devices paired");
			lv_list_add_text(m_list, "Available Devices");
			lv_list_add_text(m_list, "Tap refresh to scan");
		} else {
			lv_list_add_text(m_list, "Bluetooth is disabled");
		}
	}

	void hide() {
		if (m_container) {
			lv_obj_add_flag(m_container, LV_OBJ_FLAG_HIDDEN);
		}
	}

	void destroy() {
		m_container = nullptr;
		m_list = nullptr;
		m_btSwitch = nullptr;
		m_statusLabel = nullptr;
	}

private:

	lv_obj_t* m_parent;
	lv_obj_t* m_container = nullptr;
	lv_obj_t* m_list = nullptr;
	lv_obj_t* m_btSwitch = nullptr;
	lv_obj_t* m_statusLabel = nullptr;
	std::function<void()> m_onBack {};
};

} // namespace System::Apps::Settings
