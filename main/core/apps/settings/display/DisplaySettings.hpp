#pragma once
#include <cstring>

#include "core/apps/settings/SettingsCommon.hpp"
#include "core/system/SystemManager/SystemManager.hpp"
#include "core/ui/components/FileChooser.hpp"
#include "core/ui/theming/ThemeEngine/ThemeEngine.hpp"
#include "lvgl.h"
#include <functional>

namespace System {
namespace Apps {
namespace Settings {

class DisplaySettings {
public:

	DisplaySettings(lv_obj_t* parent, std::function<void()> onBack)
		: m_parent(parent), m_onBack(onBack) {}

	void show() {
		if (m_container == nullptr) {
			m_container = create_page_container(m_parent);

			lv_obj_t* backBtn;
			create_header(m_container, "Display", &backBtn);
			add_back_button_event_cb(backBtn, &m_onBack);

			m_list = create_settings_list(m_container);

			lv_obj_t* brightnessBtn =
				add_list_btn(m_list, LV_SYMBOL_SETTINGS, "Brightness");
			lv_obj_t* slider = lv_slider_create(brightnessBtn);
			lv_obj_set_flex_grow(slider, 1);
			lv_slider_set_range(slider, 0, 255);
			lv_slider_bind_value(
				slider, &SystemManager::getInstance().getBrightnessSubject()
			);

			lv_obj_t* themeBtn = add_list_btn(m_list, LV_SYMBOL_IMAGE, "Theme");
			lv_obj_set_flex_grow(lv_obj_get_child(themeBtn, 1), 1);

			lv_obj_t* themeValBtn = lv_button_create(themeBtn);
			lv_obj_set_size(themeValBtn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
			lv_obj_t* themeLabel = lv_label_create(themeValBtn);
			lv_label_set_text(themeLabel, Themes::ToString(ThemeEngine::get_current_theme()));

			lv_subject_add_observer_obj(
				&SystemManager::getInstance().getThemeSubject(),
				[](lv_observer_t* observer, lv_subject_t* subject) {
					lv_obj_t* label = lv_observer_get_target_obj(observer);
					if (label) {
						int32_t v = lv_subject_get_int(subject);
						lv_label_set_text(label, Themes::ToString((ThemeType)v));
					}
				},
				themeLabel, nullptr
			);

			lv_obj_add_subject_toggle_event(
				themeValBtn, &SystemManager::getInstance().getThemeSubject(),
				LV_EVENT_CLICKED
			);

			lv_obj_t* rotBtn = add_list_btn(m_list, LV_SYMBOL_REFRESH, "Rotation");
			lv_obj_set_flex_grow(lv_obj_get_child(rotBtn, 1), 1);

			lv_obj_t* rotValBtn = lv_button_create(rotBtn);
			lv_obj_set_size(rotValBtn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
			lv_obj_t* rotLabel = lv_label_create(rotValBtn);
			lv_label_bind_text(
				rotLabel, &SystemManager::getInstance().getRotationSubject(), "%d°"
			);

			lv_subject_increment_dsc_t* rot_dsc = lv_obj_add_subject_increment_event(
				rotValBtn, &SystemManager::getInstance().getRotationSubject(),
				LV_EVENT_CLICKED, 90
			);
			lv_obj_set_subject_increment_event_min_value(rotValBtn, rot_dsc, 0);
			lv_obj_set_subject_increment_event_max_value(rotValBtn, rot_dsc, 270);
			lv_obj_set_subject_increment_event_rollover(rotValBtn, rot_dsc, true);

			lv_obj_t* fpsBtn = add_list_btn(m_list, LV_SYMBOL_PLAY, "Show FPS");
			lv_obj_set_flex_grow(lv_obj_get_child(fpsBtn, 1), 1);
			lv_obj_t* fpsSw = lv_switch_create(fpsBtn);
			lv_obj_bind_checked(fpsSw, &SystemManager::getInstance().getShowFpsSubject());

			lv_obj_t* wpBtn =
				add_list_btn(m_list, LV_SYMBOL_IMAGE, "Enable Wallpaper");
			lv_obj_set_flex_grow(lv_obj_get_child(wpBtn, 1), 1);
			lv_obj_t* wpSw = lv_switch_create(wpBtn);
			lv_obj_bind_checked(
				wpSw, &SystemManager::getInstance().getWallpaperEnabledSubject()
			);

			lv_obj_t* chooseWpBtn =
				add_list_btn(m_list, LV_SYMBOL_DIRECTORY, "Choose Wallpaper");

			// Sync button state with wallpaper enablement
			auto update_chooser_state = [](lv_obj_t* btn, int32_t enabled) {
				if (enabled) {
					lv_obj_remove_state(btn, LV_STATE_DISABLED);
				} else {
					lv_obj_add_state(btn, LV_STATE_DISABLED);
				}
			};

			// Initial state
			update_chooser_state(
				chooseWpBtn,
				lv_subject_get_int(
					&SystemManager::getInstance().getWallpaperEnabledSubject()
				)
			);

			// Observer for changes
			lv_subject_add_observer_obj(
				&SystemManager::getInstance().getWallpaperEnabledSubject(),
				[](lv_observer_t* observer, lv_subject_t* subject) {
					lv_obj_t* btn = lv_observer_get_target_obj(observer);
					int32_t val = lv_subject_get_int(subject);
					if (val) {
						lv_obj_remove_state(btn, LV_STATE_DISABLED);
					} else {
						lv_obj_add_state(btn, LV_STATE_DISABLED);
					}
				},
				chooseWpBtn, nullptr
			);

			lv_obj_add_event_cb(
				chooseWpBtn,
				[](lv_event_t* e) {
					UI::FileChooser::show([](std::string path) {
						static char path_buf[256];
						strncpy(path_buf, path.c_str(), sizeof(path_buf) - 1);
						lv_subject_set_pointer(
							&SystemManager::getInstance()
								 .getWallpaperPathSubject(),
							path_buf
						);
					});
				},
				LV_EVENT_CLICKED, nullptr
			);

			lv_obj_t* glassBtn =
				add_list_btn(m_list, LV_SYMBOL_IMAGE, "Glass Effect");
			lv_obj_set_flex_grow(lv_obj_get_child(glassBtn, 1), 1);
			lv_obj_t* glassSw = lv_switch_create(glassBtn);
			lv_obj_bind_checked(
				glassSw, &SystemManager::getInstance().getGlassEnabledSubject()
			);
		} else {
			lv_obj_remove_flag(m_container, LV_OBJ_FLAG_HIDDEN);
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
	}

private:

	lv_obj_t* m_parent;
	lv_obj_t* m_container = nullptr;
	lv_obj_t* m_list = nullptr;
	std::function<void()> m_onBack;
};

} // namespace Settings
} // namespace Apps
} // namespace System
