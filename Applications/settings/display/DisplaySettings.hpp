#pragma once
#include <cstring>

#include "lvgl.h"
#include "settings/SettingsPageBase.hpp"
#include <flx/system/SystemManager.hpp>
#include <flx/system/managers/DisplayManager.hpp>
#include <flx/system/managers/ThemeManager.hpp>
#include <flx/ui/common/SettingsCommon.hpp>
#include <flx/ui/components/FileBrowser.hpp>
#include <flx/ui/theming/theme_engine/ThemeEngine.hpp>
#include <flx/ui/theming/themes/Themes.hpp>
#include <functional>
#include <memory>

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
		m_wpEnabledBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(tm.getWallpaperEnabledObservable());
		m_wpPathBridge = std::make_unique<flx::ui::LvglStringObserverBridge>(tm.getWallpaperPathObservable());
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
			themeLabel, nullptr
		);

		lv_obj_add_subject_toggle_event(
			themeValBtn, m_themeBridge->getSubject(),
			LV_EVENT_CLICKED
		);

		lv_obj_t* rotBtn = add_list_btn(m_list, LV_SYMBOL_REFRESH, "Rotation");
		lv_obj_set_flex_grow(lv_obj_get_child(rotBtn, 1), 1);

		lv_obj_t* rotValBtn = lv_button_create(rotBtn);
		lv_obj_set_size(rotValBtn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		lv_obj_t* rotLabel = lv_label_create(rotValBtn);
		lv_label_bind_text(
			rotLabel, m_rotationBridge->getSubject(), "%d°"
		);

		lv_subject_increment_dsc_t* rot_dsc = lv_obj_add_subject_increment_event(
			rotValBtn, m_rotationBridge->getSubject(),
			LV_EVENT_CLICKED, 90
		);
		lv_obj_set_subject_increment_event_min_value(rotValBtn, rot_dsc, 0);
		lv_obj_set_subject_increment_event_max_value(rotValBtn, rot_dsc, 270);
		lv_obj_set_subject_increment_event_rollover(rotValBtn, rot_dsc, true);

		lv_obj_t* fpsBtn = add_list_btn(m_list, LV_SYMBOL_PLAY, "Show FPS");
		lv_obj_set_flex_grow(lv_obj_get_child(fpsBtn, 1), 1);
		lv_obj_t* fpsSw = lv_switch_create(fpsBtn);
		lv_obj_bind_checked(fpsSw, m_fpsBridge->getSubject());

		lv_obj_t* wpBtn =
			add_list_btn(m_list, LV_SYMBOL_IMAGE, "Enable Wallpaper");
		lv_obj_set_flex_grow(lv_obj_get_child(wpBtn, 1), 1);
		lv_obj_t* wpSw = lv_switch_create(wpBtn);
		lv_obj_bind_checked(wpSw, m_wpEnabledBridge->getSubject());

		lv_obj_t* chooseWpBtn =
			add_list_btn(m_list, LV_SYMBOL_DIRECTORY, "Choose Wallpaper");

		lv_obj_set_flex_grow(lv_obj_get_child(chooseWpBtn, 1), 1);
		lv_obj_t* wpValLabel = lv_label_create(chooseWpBtn);
		lv_label_set_text(wpValLabel, "");

		lv_subject_add_observer_obj(
			m_wpPathBridge->getSubject(),
			[](lv_observer_t* observer, lv_subject_t* subject) {
				lv_obj_t* label = lv_observer_get_target_obj(observer);
				const char* path = (const char*)lv_subject_get_pointer(subject);
				if (path) {
					std::string p = path;
					size_t pos = p.find_last_of("/\\");
					std::string filename = (pos == std::string::npos) ? p : p.substr(pos + 1);
					lv_label_set_text(label, filename.c_str());
				} else {
					lv_label_set_text(label, "None");
				}
			},
			wpValLabel, nullptr
		);

		// Sync button state with wallpaper enablement
		auto update_chooser_state = [](lv_obj_t* btn, int32_t enabled) {
			if (enabled) {
				lv_obj_remove_flag(btn, LV_OBJ_FLAG_HIDDEN);
			} else {
				lv_obj_add_flag(btn, LV_OBJ_FLAG_HIDDEN);
			}
		};

		// Initial state
		update_chooser_state(
			chooseWpBtn,
			lv_subject_get_int(m_wpEnabledBridge->getSubject())
		);

		// Observer for changes
		lv_subject_add_observer_obj(
			m_wpEnabledBridge->getSubject(),
			[](lv_observer_t* observer, lv_subject_t* subject) {
				lv_obj_t* btn = lv_observer_get_target_obj(observer);
				int32_t const val = lv_subject_get_int(subject);
				if (val) {
					lv_obj_remove_flag(btn, LV_OBJ_FLAG_HIDDEN);
				} else {
					lv_obj_add_flag(btn, LV_OBJ_FLAG_HIDDEN);
				}
			},
			chooseWpBtn, nullptr
		);

		lv_obj_add_event_cb(
			chooseWpBtn,
			[](lv_event_t* e) {
				auto* self = static_cast<DisplaySettings*>(lv_event_get_user_data(e));
				if (!self->m_fileBrowser) {
					self->m_fileBrowser = new flx::ui::FileBrowser(self->m_parent, [self]() {
						self->m_fileBrowser->hide();
					});
				}
				self->m_fileBrowser->setExtensions({".png", ".jpg", ".jpeg", ".bmp"});
				self->m_fileBrowser->show(false, [self](const std::string& path) {
					static char path_buf[256];
					strncpy(path_buf, path.c_str(), sizeof(path_buf) - 1);
					path_buf[sizeof(path_buf) - 1] = '\0';
					lv_subject_set_pointer(
						self->m_wpPathBridge->getSubject(),
						path_buf
					);
					self->m_fileBrowser->hide();
				});
			},
			LV_EVENT_CLICKED, this
		);

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
			glassSw, nullptr
		);
	}

	void onDestroy() override {
		if (m_fileBrowser) {
			delete m_fileBrowser;
			m_fileBrowser = nullptr;
		}
	}

private:

	flx::ui::FileBrowser* m_fileBrowser {nullptr};

	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_brightnessBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_rotationBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_fpsBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_themeBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_wpEnabledBridge;
	std::unique_ptr<flx::ui::LvglStringObserverBridge> m_wpPathBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_transpBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_glassBridge;
};

} // namespace System::Apps::Settings
