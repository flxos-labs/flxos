#pragma once

#include <flx/apps/AppManager.hpp>
#include <flx/apps/Intent.hpp>
#include <flx/system/SystemManager.hpp>
#include <flx/system/managers/DisplayManager.hpp>
#include <flx/system/managers/ThemeManager.hpp>
#include <flx/ui/theming/theme_engine/ThemeEngine.hpp>
#include <flx/ui/theming/themes/Themes.hpp>
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
		auto& tm = ThemeManager::getInstance();

		m_brightnessBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(dm.getBrightnessObservable());
		m_rotationBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(dm.getRotationObservable());
		m_fpsBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(dm.getShowFpsObservable());
		m_themeBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(tm.getThemeObservable());
		m_transpBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(tm.getTransparencyEnabledObservable());
		m_glassBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(tm.getGlassEnabledObservable());

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

		lv_obj_t* themeBtn = add_list_btn(m_list, LV_SYMBOL_IMAGE, "Theme");
		lv_obj_set_flex_grow(lv_obj_get_child(themeBtn, 1), 1);

		lv_obj_t* themeValBtn = lv_button_create(themeBtn);
		lv_obj_set_size(themeValBtn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		lv_obj_t* themeLabel = lv_label_create(themeValBtn);
		lv_label_set_text(themeLabel, Themes::ToString(ThemeEngine::get_current_theme()));

		lv_subject_add_observer_obj(
			m_themeBridge->getSubject(),
			[](lv_observer_t* observer, lv_subject_t* subject) {
				lv_obj_t* label = lv_observer_get_target_obj(observer);
				if (label) {
					int32_t const v = lv_subject_get_int(subject);
					lv_label_set_text(label, Themes::ToString((ThemeType)v));
				}
			},
			themeLabel, nullptr);

		lv_obj_add_subject_toggle_event(
			themeValBtn, m_themeBridge->getSubject(),
			LV_EVENT_CLICKED);

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

		lv_obj_t* wallpaperEngineBtn =
			add_list_btn(m_list, LV_SYMBOL_IMAGE, "Wallpaper");
		lv_obj_set_flex_grow(lv_obj_get_child(wallpaperEngineBtn, 1), 1);
		lv_obj_t* wallpaperEngineHintBtn = lv_button_create(wallpaperEngineBtn);
		lv_obj_set_size(wallpaperEngineHintBtn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		lv_obj_set_style_border_width(wallpaperEngineHintBtn, 1, 0);
		lv_obj_set_style_border_side(wallpaperEngineHintBtn, LV_BORDER_SIDE_FULL, 0);
		lv_obj_set_style_pad_hor(wallpaperEngineHintBtn, 8, 0);
		lv_obj_set_style_pad_ver(wallpaperEngineHintBtn, 2, 0);
		lv_obj_add_flag(wallpaperEngineHintBtn, LV_OBJ_FLAG_EVENT_BUBBLE);

		lv_obj_t* wallpaperEngineHint = lv_label_create(wallpaperEngineHintBtn);
		lv_label_set_text(wallpaperEngineHint, "Open App");
		lv_obj_set_style_text_opa(wallpaperEngineHint, LV_OPA_70, 0);

		lv_obj_add_event_cb(
			wallpaperEngineBtn,
			[](lv_event_t* /*e*/) {
				flx::apps::AppManager::getInstance().startApp(
					flx::apps::Intent::forApp("com.flxos.wallpaper_engine"));
			},
			LV_EVENT_CLICKED, nullptr);

		lv_obj_t* transpBtn =
			add_list_btn(m_list, LV_SYMBOL_EYE_OPEN, "Transparency");
		lv_obj_set_flex_grow(lv_obj_get_child(transpBtn, 1), 1);
		lv_obj_t* transpSw = lv_switch_create(transpBtn);
		lv_obj_bind_checked(transpSw, m_transpBridge->getSubject());

		lv_obj_t* glassBtn =
			add_list_btn(m_list, LV_SYMBOL_IMAGE, "Glass Effect");
		lv_obj_set_flex_grow(lv_obj_get_child(glassBtn, 1), 1);
		lv_obj_t* glassSw = lv_switch_create(glassBtn);
		lv_obj_bind_checked(glassSw, m_glassBridge->getSubject());

		// observer to disable glass setting if transparency is off
		lv_subject_add_observer_obj(
			m_transpBridge->getSubject(),
			[](lv_observer_t* observer, lv_subject_t* subject) {
				lv_obj_t* glassSw = lv_observer_get_target_obj(observer);
				lv_obj_t* glassBtn = lv_obj_get_parent(glassSw);
				int32_t const enabled = lv_subject_get_int(subject);
				if (enabled) {
					lv_obj_remove_state(glassBtn, LV_STATE_DISABLED);
					lv_obj_remove_state(glassSw, LV_STATE_DISABLED);
					lv_obj_set_style_opa(glassBtn, LV_OPA_COVER, 0);
				} else {
					lv_obj_add_state(glassBtn, LV_STATE_DISABLED);
					lv_obj_add_state(glassSw, LV_STATE_DISABLED);
					lv_obj_set_style_opa(glassBtn, LV_OPA_50, 0);
				}
			},
			glassSw, nullptr);
	}

private:

	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_brightnessBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_rotationBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_fpsBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_themeBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_transpBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_glassBridge;
};

} // namespace System::Apps::Settings
