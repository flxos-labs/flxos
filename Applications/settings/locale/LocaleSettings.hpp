#pragma once

#include <ctime>
#include <flx/core/Observable.hpp>
#include <flx/system/managers/SettingsManager.hpp>
#include <flx/ui/LvglObserverBridge.hpp>
#include <lvgl.h>
#include <memory>
#include <string>

#include "settings/SettingsPageBase.hpp"

using namespace flx::ui::common;
using namespace flx::system;

namespace System::Apps::Settings {

class LocaleSettings : public SettingsPageBase {
public:

	using SettingsPageBase::SettingsPageBase;

protected:

	void createUI() override {
		auto& sm = SettingsManager::getInstance();

		m_timeFormatBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(sm.getTimeFormat24h());

		m_container = create_page_container(m_parent);

		lv_obj_t* backBtn = nullptr;
		create_header(m_container, "Language & Region", &backBtn);
		add_back_button_event_cb(backBtn, &m_onBack);

		m_list = create_settings_list(m_container);

		// ── Language Section ──
		lv_list_add_text(m_list, "Language");

		lv_obj_t* langBtn = add_list_btn(m_list, LV_SYMBOL_AUDIO, "System Language");
		lv_obj_set_flex_grow(lv_obj_get_child(langBtn, 1), 1);

		m_langDropdown = lv_dropdown_create(langBtn);
		lv_dropdown_set_options(m_langDropdown, "English\nEspañol\nDeutsch\nFrançais\nItaliano");
		lv_dropdown_set_dir(m_langDropdown, LV_DIR_LEFT);
		lv_dropdown_set_selected(m_langDropdown, sm.getLanguageIndex().get());
		lv_obj_add_event_cb(
			m_langDropdown,
			[](lv_event_t* e) {
				if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
					auto* dropdown = lv_event_get_target_obj(e);
					SettingsManager::getInstance().getLanguageIndex().set(lv_dropdown_get_selected(dropdown));
				}
			},
			LV_EVENT_VALUE_CHANGED, nullptr);

		// ── Time & Date Formats ──
		lv_list_add_text(m_list, "Formats");

		lv_obj_t* timeFormatBtn = add_list_btn(m_list, LV_SYMBOL_SETTINGS, "Use 24-Hour Time");
		lv_obj_set_flex_grow(lv_obj_get_child(timeFormatBtn, 1), 1);
		lv_obj_t* timeFormatSw = lv_switch_create(timeFormatBtn);
		lv_obj_add_flag(timeFormatSw, LV_OBJ_FLAG_EVENT_BUBBLE);
		lv_obj_bind_checked(timeFormatSw, m_timeFormatBridge->getSubject());

		lv_obj_t* dateFormatBtn = add_list_btn(m_list, LV_SYMBOL_DIRECTORY, "Date Format");
		lv_obj_set_flex_grow(lv_obj_get_child(dateFormatBtn, 1), 1);

		m_dateDropdown = lv_dropdown_create(dateFormatBtn);
		lv_dropdown_set_options(m_dateDropdown, "YYYY-MM-DD\nDD/MM/YYYY\nMM/DD/YYYY");
		lv_dropdown_set_dir(m_dateDropdown, LV_DIR_LEFT);
		lv_dropdown_set_selected(m_dateDropdown, sm.getDateFormatIndex().get());
		lv_obj_add_event_cb(
			m_dateDropdown,
			[](lv_event_t* e) {
				if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
					auto* dropdown = lv_event_get_target_obj(e);
					auto* instance = static_cast<LocaleSettings*>(lv_event_get_user_data(e));
					int32_t const sel = lv_dropdown_get_selected(dropdown);
					SettingsManager::getInstance().getDateFormatIndex().set(sel);
					if (instance) {
						instance->updateDatePreview();
					}
				}
			},
			LV_EVENT_VALUE_CHANGED, this);

		// ── Date Preview ──
		lv_obj_t* previewBtn = add_list_btn(m_list, LV_SYMBOL_EYE_OPEN, "Date Preview");
		lv_obj_set_flex_grow(lv_obj_get_child(previewBtn, 1), 1);
		m_previewLabel = lv_label_create(previewBtn);
		updateDatePreview();
	}

	void onDestroy() override {
		m_timeFormatBridge.reset();
		m_langDropdown = nullptr;
		m_dateDropdown = nullptr;
		m_previewLabel = nullptr;
	}

private:

	void updateDatePreview() {
		if (!m_previewLabel) return;
		int32_t const formatIndex = SettingsManager::getInstance().getDateFormatIndex().get();
		time_t now = 0;
		struct tm timeinfo = {};
		time(&now);
		localtime_r(&now, &timeinfo);
		char buf[64];
		if (formatIndex == 0) {
			snprintf(buf, sizeof(buf), "%04d-%02d-%02d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
		} else if (formatIndex == 1) {
			snprintf(buf, sizeof(buf), "%02d/%02d/%04d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
		} else if (formatIndex == 2) {
			snprintf(buf, sizeof(buf), "%02d/%02d/%04d", timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_year + 1900);
		} else {
			snprintf(buf, sizeof(buf), "%04d-%02d-%02d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
		}
		lv_label_set_text(m_previewLabel, buf);
	}

	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_timeFormatBridge;
	lv_obj_t* m_langDropdown = nullptr;
	lv_obj_t* m_dateDropdown = nullptr;
	lv_obj_t* m_previewLabel = nullptr;
};

} // namespace System::Apps::Settings
