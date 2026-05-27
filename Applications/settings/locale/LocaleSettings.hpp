#pragma once

#include <flx/core/Observable.hpp>
#include <flx/system/managers/SettingsManager.hpp>
#include <flx/ui/LvglObserverBridge.hpp>
#include <lvgl.h>
#include <memory>

#include "settings/SettingsPageBase.hpp"

using namespace flx::ui::common;
using namespace flx::system;

namespace System::Apps::Settings {

class LocaleSettings : public SettingsPageBase {
public:

	using SettingsPageBase::SettingsPageBase;

	static inline flx::Observable<int32_t> languageIndex {0};
	static inline flx::Observable<int32_t> timeFormat24h {1};
	static inline flx::Observable<int32_t> dateFormatIndex {0};

protected:

	void createUI() override {
		static bool registered = false;
		if (!registered) {
			SettingsManager::getInstance().registerSetting("locale.language_index", languageIndex);
			SettingsManager::getInstance().registerSetting("locale.time_format_24h", timeFormat24h);
			SettingsManager::getInstance().registerSetting("locale.date_format_index", dateFormatIndex);
			registered = true;
		}

		m_timeFormatBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(timeFormat24h);

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
		lv_dropdown_set_selected(m_langDropdown, languageIndex.get());
		lv_obj_add_event_cb(
			m_langDropdown,
			[](lv_event_t* e) {
				if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
					auto* dropdown = lv_event_get_target_obj(e);
					languageIndex.set(lv_dropdown_get_selected(dropdown));
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
		lv_dropdown_set_selected(m_dateDropdown, dateFormatIndex.get());
		lv_obj_add_event_cb(
			m_dateDropdown,
			[](lv_event_t* e) {
				if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
					auto* dropdown = lv_event_get_target_obj(e);
					dateFormatIndex.set(lv_dropdown_get_selected(dropdown));
				}
			},
			LV_EVENT_VALUE_CHANGED, nullptr);
	}

	void onDestroy() override {
		m_timeFormatBridge.reset();
		m_langDropdown = nullptr;
		m_dateDropdown = nullptr;
	}

private:

	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_timeFormatBridge;
	lv_obj_t* m_langDropdown = nullptr;
	lv_obj_t* m_dateDropdown = nullptr;
};

} // namespace System::Apps::Settings
