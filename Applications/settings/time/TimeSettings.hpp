#pragma once

#include <array>
#include <cstring>
#include <flx/system/managers/TimeManager.hpp>
#include <lvgl.h>
#include <string>

#include "settings/SettingsPageBase.hpp"

using namespace flx::ui::common;
using namespace flx::system;

namespace System::Apps::Settings {

class TimeSettings : public SettingsPageBase {
public:

	using SettingsPageBase::SettingsPageBase;

protected:

	void createUI() override {
		m_container = create_page_container(m_parent);

		lv_obj_t* backBtn = nullptr;
		create_header(m_container, "Date & Time", &backBtn);
		add_back_button_event_cb(backBtn, &m_onBack);

		m_list = create_settings_list(m_container);

		// ── NTP Sync Status ──
		lv_list_add_text(m_list, "NTP Time Sync");

		lv_obj_t* syncBtn = add_list_btn(m_list, LV_SYMBOL_REFRESH, "Sync Status");
		lv_obj_set_flex_grow(lv_obj_get_child(syncBtn, 1), 1);
		m_syncStatusLabel = lv_label_create(syncBtn);
		updateSyncStatus();

		// ── Sync Now button ──
		lv_obj_t* syncNowBtn = add_list_btn(m_list, LV_SYMBOL_REFRESH, "Sync Now");
		lv_obj_add_event_cb(
			syncNowBtn,
			[](lv_event_t* e) {
				auto* instance = (TimeSettings*)lv_event_get_user_data(e);
				if (!instance) {
					return;
				}
				TimeManager::getInstance().syncTime();
				// Refresh status after a short delay
				if (instance->m_syncTimer) {
					lv_timer_delete(instance->m_syncTimer);
					instance->m_syncTimer = nullptr;
				}
				instance->m_syncTimer = lv_timer_create(
					[](lv_timer_t* t) {
						auto* inst = (TimeSettings*)lv_timer_get_user_data(t);
						if (inst) {
							inst->updateSyncStatus();
							inst->m_syncTimer = nullptr;
						}
						lv_timer_delete(t);
					},
					500, instance);
			},
			LV_EVENT_CLICKED, this);

		// ── Timezone ──
		lv_list_add_text(m_list, "Timezone");

		// Preset timezone dropdown
		lv_obj_t* tzPresetBtn = add_list_btn(m_list, LV_SYMBOL_GPS, "Quick Select");
		lv_obj_set_flex_grow(lv_obj_get_child(tzPresetBtn, 1), 1);

		m_tzDropdown = lv_dropdown_create(tzPresetBtn);
		lv_dropdown_set_options(
			m_tzDropdown,
			"UTC+0 (GMT)\n"
			"UTC+5:30 (IST)\n"
			"UTC-5 (EST)\n"
			"UTC-8 (PST)\n"
			"UTC+1 (CET)\n"
			"UTC+9 (JST)\n"
			"UTC+8 (CST)\n"
			"UTC+10 (AEST)\n"
			"UTC+3 (MSK)\n"
			"UTC-3 (BRT)");
		lv_dropdown_set_dir(m_tzDropdown, LV_DIR_LEFT);
		lv_obj_set_width(m_tzDropdown, LV_SIZE_CONTENT);

		lv_obj_add_event_cb(
			m_tzDropdown,
			[](lv_event_t* e) {
				auto* instance = (TimeSettings*)lv_event_get_user_data(e);
				auto* dd = lv_event_get_target_obj(e);
				uint32_t const sel = lv_dropdown_get_selected(dd);
				// POSIX TZ strings for each preset
				static const char* kTzStrings[] = {
					"UTC0", // UTC/GMT
					"IST-5:30", // India Standard Time
					"EST5EDT,M3.2.0,M11.1.0", // US Eastern
					"PST8PDT,M3.2.0,M11.1.0", // US Pacific
					"CET-1CEST,M3.5.0,M10.5.0/3", // Central Europe
					"JST-9", // Japan
					"CST-8", // China
					"AEST-10AEDT,M10.1.0,M4.1.0/3", // Australia East
					"MSK-3", // Moscow
					"<-03>3", // Brazil
				};
				if (sel < 10) {
					TimeManager::setTimeZone(kTzStrings[sel]);
					// Update custom TA to reflect selection
					if (instance->m_tzTextArea) {
						lv_textarea_set_text(instance->m_tzTextArea, kTzStrings[sel]);
					}
				}
			},
			LV_EVENT_VALUE_CHANGED, this);

		// Custom POSIX timezone string input
		lv_list_add_text(m_list, "Custom POSIX TZ String");
		m_tzTextArea = lv_textarea_create(m_list);
		lv_textarea_set_one_line(m_tzTextArea, true);
		lv_textarea_set_placeholder_text(m_tzTextArea, "e.g. IST-5:30 or EST5EDT,...");
		lv_obj_set_width(m_tzTextArea, lv_pct(90));
		lv_textarea_set_text(m_tzTextArea, "UTC0");

		lv_obj_t* applyTzBtn = lv_list_add_button(m_list, LV_SYMBOL_OK, "Apply Timezone");
		lv_obj_add_event_cb(
			applyTzBtn,
			[](lv_event_t* e) {
				auto* instance = (TimeSettings*)lv_event_get_user_data(e);
				if (!instance || !instance->m_tzTextArea) {
					return;
				}
				const char* tz = lv_textarea_get_text(instance->m_tzTextArea);
				if (tz && strlen(tz) > 0) {
					TimeManager::setTimeZone(tz);
				}
			},
			LV_EVENT_CLICKED, this);
	}

	void onShow() override {
		updateSyncStatus();
	}

	void onDestroy() override {
		if (m_syncTimer) {
			lv_timer_delete(m_syncTimer);
			m_syncTimer = nullptr;
		}
		m_syncStatusLabel = nullptr;
		m_tzDropdown = nullptr;
		m_tzTextArea = nullptr;
	}

private:

	void updateSyncStatus() {
		if (!m_syncStatusLabel) {
			return;
		}
		bool const synced = TimeManager::getInstance().isSynced();
		lv_label_set_text(m_syncStatusLabel, synced ? "Synced" : "Not Synced");
	}

	lv_obj_t* m_syncStatusLabel = nullptr;
	lv_obj_t* m_tzDropdown = nullptr;
	lv_obj_t* m_tzTextArea = nullptr;
	lv_timer_t* m_syncTimer = nullptr;
};

} // namespace System::Apps::Settings
