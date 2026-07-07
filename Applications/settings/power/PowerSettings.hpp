#pragma once

#include <flx/hal/DeviceRegistry.hpp>
#include <flx/hal/power/IPowerDevice.hpp>
#include <flx/system/managers/PowerManager.hpp>
#include <flx/ui/LvglObserverBridge.hpp>
#include <lvgl.h>
#include <memory>

#include "settings/SettingsPageBase.hpp"

using namespace flx::ui::common;
using namespace flx::system;

namespace System::Apps::Settings {

class PowerSettings : public SettingsPageBase {
public:

	using SettingsPageBase::SettingsPageBase;

protected:

	void createUI() override {
		auto& pm = PowerManager::getInstance();

		m_batteryBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(pm.getBatteryLevelObservable());
		m_chargingBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(pm.getIsChargingObservable());
		m_configuredBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(pm.getIsConfiguredObservable());

		m_container = create_page_container(m_parent);

		lv_obj_t* backBtn = nullptr;
		create_header(m_container, "Battery & Power", &backBtn);
		add_back_button_event_cb(backBtn, &m_onBack);

		m_list = create_settings_list(m_container);

		// ── Battery Level row ──
		lv_obj_t* levelBtn = add_list_btn(m_list, LV_SYMBOL_BATTERY_FULL, "Battery Level");
		lv_obj_set_flex_grow(lv_obj_get_child(levelBtn, 1), 1);

		m_levelLabel = lv_label_create(levelBtn);
		lv_label_set_text(m_levelLabel, "-- %");

		lv_subject_add_observer_obj(
			m_batteryBridge->getSubject(),
			[](lv_observer_t* observer, lv_subject_t* subject) {
				auto* label = lv_observer_get_target_obj(observer);
				auto* instance = (PowerSettings*)lv_observer_get_user_data(observer);
				if (!label || !instance) {
					return;
				}
				int32_t const pct = lv_subject_get_int(subject);
				int32_t const configured = lv_subject_get_int(instance->m_configuredBridge->getSubject());
				if (configured) {
					lv_label_set_text_fmt(label, "%ld%%", pct);
					// Update battery icon based on level
					lv_obj_t* icon = lv_obj_get_child(lv_obj_get_parent(label), 0);
					if (icon) {
						if (pct > 75) {
							lv_image_set_src(icon, LV_SYMBOL_BATTERY_FULL);
						} else if (pct > 50) {
							lv_image_set_src(icon, LV_SYMBOL_BATTERY_3);
						} else if (pct > 25) {
							lv_image_set_src(icon, LV_SYMBOL_BATTERY_2);
						} else if (pct > 10) {
							lv_image_set_src(icon, LV_SYMBOL_BATTERY_1);
						} else {
							lv_image_set_src(icon, LV_SYMBOL_BATTERY_EMPTY);
						}
					}
				} else {
					lv_label_set_text(label, "N/A");
				}
			},
			m_levelLabel, this);

		// ── Charging Status row ──
		lv_obj_t* chargingBtn = add_list_btn(m_list, LV_SYMBOL_CHARGE, "Charging");
		lv_obj_set_flex_grow(lv_obj_get_child(chargingBtn, 1), 1);

		m_chargingLabel = lv_label_create(chargingBtn);
		lv_label_set_text(m_chargingLabel, "--");

		lv_subject_add_observer_obj(
			m_chargingBridge->getSubject(),
			[](lv_observer_t* observer, lv_subject_t* subject) {
				auto* label = lv_observer_get_target_obj(observer);
				auto* instance = (PowerSettings*)lv_observer_get_user_data(observer);
				if (!label || !instance) {
					return;
				}
				int32_t const configured = lv_subject_get_int(instance->m_configuredBridge->getSubject());
				if (configured) {
					bool const charging = lv_subject_get_int(subject) != 0;
					lv_label_set_text(label, charging ? "Charging" : "Not Charging");
				} else {
					lv_label_set_text(label, "N/A");
				}
			},
			m_chargingLabel, this);

		// Add status notice label (shown when no power device is configured)
		lv_list_add_text(m_list, "Power Device");
		lv_obj_t* noticeCont = lv_list_add_button(m_list, LV_SYMBOL_WARNING, "No power device configured");
		lv_obj_set_style_text_opa(lv_obj_get_child(noticeCont, 1), LV_OPA_60, 0);
		m_noticeItem = noticeCont;

		m_deviceItem = lv_list_add_button(m_list, LV_SYMBOL_SETTINGS, "Power Controller");
		lv_obj_add_flag(m_deviceItem, LV_OBJ_FLAG_HIDDEN);

		// Wire the configure observer to the notice item now that it's created
		lv_subject_add_observer_obj(
			m_configuredBridge->getSubject(),
			[](lv_observer_t* observer, lv_subject_t* subject) {
				auto* instance = (PowerSettings*)lv_observer_get_user_data(observer);
				if (!instance) {
					return;
				}
				bool const configured = lv_subject_get_int(subject) != 0;
				if (configured) {
					if (instance->m_noticeItem) lv_obj_add_flag(instance->m_noticeItem, LV_OBJ_FLAG_HIDDEN);

					auto powerDev = flx::hal::DeviceRegistry::getInstance()
										.findFirst<flx::hal::power::IPowerDevice>(flx::hal::IDevice::Type::Power);
					std::string name = "Power Controller";
					if (powerDev) {
						name = std::string(powerDev->getName());
					}
					if (instance->m_deviceItem) {
						lv_obj_t* label = lv_obj_get_child(instance->m_deviceItem, 1);
						if (label) {
							lv_label_set_text(label, name.c_str());
						}
						lv_obj_remove_flag(instance->m_deviceItem, LV_OBJ_FLAG_HIDDEN);
					}
				} else {
					if (instance->m_noticeItem) lv_obj_remove_flag(instance->m_noticeItem, LV_OBJ_FLAG_HIDDEN);
					if (instance->m_deviceItem) lv_obj_add_flag(instance->m_deviceItem, LV_OBJ_FLAG_HIDDEN);
				}
			},
			m_noticeItem, this);
	}

	void onShow() override {
		PowerManager::getInstance().refresh();
	}

	void onDestroy() override {
		m_levelLabel = nullptr;
		m_chargingLabel = nullptr;
		m_noticeItem = nullptr;
		m_deviceItem = nullptr;
	}

private:

	lv_obj_t* m_levelLabel = nullptr;
	lv_obj_t* m_chargingLabel = nullptr;
	lv_obj_t* m_noticeItem = nullptr;
	lv_obj_t* m_deviceItem = nullptr;

	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_batteryBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_chargingBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_configuredBridge;
};

} // namespace System::Apps::Settings
