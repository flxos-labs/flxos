#pragma once
#include <cstring>

#include "core/apps/settings/SettingsCommon.hpp"
#include "core/apps/settings/SettingsPageBase.hpp"
#include "core/system/display/DisplayManager.hpp"
#include "core/system/system_core/SystemManager.hpp"
#include "core/system/theme/ThemeManager.hpp"
#include "core/ui/components/FileBrowser.hpp"
#include "core/ui/theming/theme_engine/ThemeEngine.hpp"
#include "lvgl.h"
#include <functional>

namespace System::Apps::Settings {

class DisplaySettings : public SettingsPageBase {
public:

	using SettingsPageBase::SettingsPageBase;

protected:

	void createUI() override {
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
		lv_slider_bind_value(
			slider, &DisplayManager::getInstance().getBrightnessSubject()
		);

		lv_obj_t* themeBtn = add_list_btn(m_list, LV_SYMBOL_IMAGE, "Theme");
		lv_obj_set_flex_grow(lv_obj_get_child(themeBtn, 1), 1);

		lv_obj_t* themeValBtn = lv_button_create(themeBtn);
		lv_obj_set_size(themeValBtn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		lv_obj_t* themeLabel = lv_label_create(themeValBtn);
		lv_label_set_text(themeLabel, Themes::ToString(ThemeEngine::get_current_theme()));

		lv_subject_add_observer_obj(
			&ThemeManager::getInstance().getThemeSubject(),
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
			themeValBtn, &ThemeManager::getInstance().getThemeSubject(),
			LV_EVENT_CLICKED
		);

		lv_obj_t* rotBtn = add_list_btn(m_list, LV_SYMBOL_REFRESH, "Rotation");
		lv_obj_set_flex_grow(lv_obj_get_child(rotBtn, 1), 1);

		lv_obj_t* rotValBtn = lv_button_create(rotBtn);
		lv_obj_set_size(rotValBtn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		lv_obj_t* rotLabel = lv_label_create(rotValBtn);
		lv_label_bind_text(
			rotLabel, &DisplayManager::getInstance().getRotationSubject(), "%d°"
		);

		lv_subject_increment_dsc_t* rot_dsc = lv_obj_add_subject_increment_event(
			rotValBtn, &DisplayManager::getInstance().getRotationSubject(),
			LV_EVENT_CLICKED, 90
		);
		lv_obj_set_subject_increment_event_min_value(rotValBtn, rot_dsc, 0);
		lv_obj_set_subject_increment_event_max_value(rotValBtn, rot_dsc, 270);
		lv_obj_set_subject_increment_event_rollover(rotValBtn, rot_dsc, true);

		lv_obj_t* fpsBtn = add_list_btn(m_list, LV_SYMBOL_PLAY, "Show FPS");
		lv_obj_set_flex_grow(lv_obj_get_child(fpsBtn, 1), 1);
		lv_obj_t* fpsSw = lv_switch_create(fpsBtn);
		lv_obj_bind_checked(fpsSw, &DisplayManager::getInstance().getShowFpsSubject());

		lv_obj_t* wpBtn =
			add_list_btn(m_list, LV_SYMBOL_IMAGE, "Enable Wallpaper");
		lv_obj_set_flex_grow(lv_obj_get_child(wpBtn, 1), 1);
		lv_obj_t* wpSw = lv_switch_create(wpBtn);
		lv_obj_bind_checked(
			wpSw, &ThemeManager::getInstance().getWallpaperEnabledSubject()
		);

		lv_obj_t* chooseWpBtn =
			add_list_btn(m_list, LV_SYMBOL_DIRECTORY, "Choose Wallpaper");

		lv_obj_set_flex_grow(lv_obj_get_child(chooseWpBtn, 1), 1);
		lv_obj_t* wpValLabel = lv_label_create(chooseWpBtn);
		lv_label_set_text(wpValLabel, "");

		lv_subject_add_observer_obj(
			&ThemeManager::getInstance().getWallpaperPathSubject(),
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
			lv_subject_get_int(
				&ThemeManager::getInstance().getWallpaperEnabledSubject()
			)
		);

		// Observer for changes
		lv_subject_add_observer_obj(
			&ThemeManager::getInstance().getWallpaperEnabledSubject(),
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
					self->m_fileBrowser = new UI::FileBrowser(self->m_parent, [self]() {
						self->m_fileBrowser->hide();
					});
				}
				self->m_fileBrowser->setExtensions({".png", ".jpg", ".jpeg", ".bmp"});
				self->m_fileBrowser->show(false, [self](const std::string& path) {
					static char path_buf[256];
					strncpy(path_buf, path.c_str(), sizeof(path_buf) - 1);
					path_buf[sizeof(path_buf) - 1] = '\0';
					lv_subject_set_pointer(
						&ThemeManager::getInstance().getWallpaperPathSubject(),
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
		lv_obj_bind_checked(
			transpSw,
			&ThemeManager::getInstance().getTransparencyEnabledSubject()
		);

		lv_obj_t* glassBtn =
			add_list_btn(m_list, LV_SYMBOL_IMAGE, "Glass Effect");
		lv_obj_set_flex_grow(lv_obj_get_child(glassBtn, 1), 1);
		lv_obj_t* glassSw = lv_switch_create(glassBtn);
		lv_obj_bind_checked(
			glassSw, &ThemeManager::getInstance().getGlassEnabledSubject()
		);

		// observer to disable glass setting if transparency is off
		lv_subject_add_observer_obj(
			&ThemeManager::getInstance().getTransparencyEnabledSubject(),
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

	UI::FileBrowser* m_fileBrowser {nullptr};
};

} // namespace System::Apps::Settings
