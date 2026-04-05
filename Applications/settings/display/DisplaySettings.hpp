#pragma once

#include <flx/system/managers/DisplayManager.hpp>
#include <flx/ui/LvglObserverBridge.hpp>
#include <lvgl.h>
#include <memory>

#include "settings/SettingsPageBase.hpp"

using namespace flx::ui::common;
using namespace flx::system;

namespace System::Apps::Settings {

class DisplaySettings : public SettingsPageBase {
public:

	using SettingsPageBase::SettingsPageBase;

protected:

	void createUI() override {
		auto& dm = DisplayManager::getInstance();

		m_brightnessBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(dm.getBrightnessObservable());
		m_rotationBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(dm.getRotationObservable());
		m_fpsBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(dm.getShowFpsObservable());

		m_container = create_page_container(m_parent);

		lv_obj_t* backBtn = nullptr;
		create_header(m_container, "Display", &backBtn);
		add_back_button_event_cb(backBtn, &m_onBack);

		m_list = create_settings_list(m_container);

		lv_obj_t* brightnessBtn =
			add_list_btn(m_list, LV_SYMBOL_SETTINGS, "Brightness");
		lv_obj_t* slider = lv_slider_create(brightnessBtn);
		lv_obj_set_flex_grow(slider, 1);
		lv_slider_set_range(slider, 0, 255);
		lv_slider_bind_value(slider, m_brightnessBridge->getSubject());

		lv_obj_t* rotBtn = add_list_btn(m_list, LV_SYMBOL_REFRESH, "Rotation");
		lv_obj_set_flex_grow(lv_obj_get_child(rotBtn, 1), 1);

		lv_obj_t* rotValBtn = lv_button_create(rotBtn);
		lv_obj_set_size(rotValBtn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		lv_obj_t* rotLabel = lv_label_create(rotValBtn);
		lv_label_bind_text(
			rotLabel, m_rotationBridge->getSubject(), "%d°");

		lv_subject_increment_dsc_t* rot_dsc = lv_obj_add_subject_increment_event(
			rotValBtn, m_rotationBridge->getSubject(),
			LV_EVENT_CLICKED, 90);
		lv_obj_set_subject_increment_event_min_value(rotValBtn, rot_dsc, 0);
		lv_obj_set_subject_increment_event_max_value(rotValBtn, rot_dsc, 270);
		lv_obj_set_subject_increment_event_rollover(rotValBtn, rot_dsc, true);

		lv_obj_t* fpsBtn = add_list_btn(m_list, LV_SYMBOL_PLAY, "Show FPS");
		lv_obj_set_flex_grow(lv_obj_get_child(fpsBtn, 1), 1);
		lv_obj_t* fpsSw = lv_switch_create(fpsBtn);
		lv_obj_bind_checked(fpsSw, m_fpsBridge->getSubject());
	}

private:

	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_brightnessBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_rotationBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_fpsBridge;
};

} // namespace System::Apps::Settings
