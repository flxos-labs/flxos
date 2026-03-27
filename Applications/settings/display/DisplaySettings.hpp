#pragma once
#include <cstdio>
#include <cstring>

#include "lvgl.h"
#include "settings/SettingsPageBase.hpp"
#include <flx/system/SystemManager.hpp>
#include <flx/system/managers/DisplayManager.hpp>
#include <flx/system/managers/ThemeManager.hpp>
#include <flx/system/managers/WallpaperManager.hpp>
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
		auto& wm = flx::system::WallpaperManager::getInstance();

		auto parse_int_effect = [&wm](const std::string& key, int32_t fallback) {
			std::string const effects = wm.getWallpaperEffectsObservable().get();
			std::string const needle = "\"" + key + "\":";
			size_t const pos = effects.find(needle);
			if (pos == std::string::npos) {
				return fallback;
			}

			char* end_ptr = nullptr;
			long const parsed = std::strtol(effects.c_str() + pos + needle.size(), &end_ptr, 10);
			if (end_ptr == nullptr || end_ptr == effects.c_str() + pos + needle.size()) {
				return fallback;
			}
			return static_cast<int32_t>(parsed);
		};

		auto parse_float_effect = [&wm](const std::string& key, float fallback) {
			std::string const effects = wm.getWallpaperEffectsObservable().get();
			std::string const needle = "\"" + key + "\":";
			size_t const pos = effects.find(needle);
			if (pos == std::string::npos) {
				return fallback;
			}

			char* end_ptr = nullptr;
			float const parsed = std::strtof(effects.c_str() + pos + needle.size(), &end_ptr);
			if (end_ptr == nullptr || end_ptr == effects.c_str() + pos + needle.size()) {
				return fallback;
			}
			return parsed;
		};

		auto has_true_effect = [&wm](const std::string& key) {
			std::string const effects = wm.getWallpaperEffectsObservable().get();
			std::string const needle = "\"" + key + "\":true";
			return effects.find(needle) != std::string::npos;
		};

		m_brightnessBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(dm.getBrightnessObservable());
		m_rotationBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(dm.getRotationObservable());
		m_fpsBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(dm.getShowFpsObservable());
		m_themeBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(tm.getThemeObservable());
		m_wpEnabledBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(wm.getWallpaperEnabledObservable());
		m_wpPathBridge = std::make_unique<flx::ui::LvglStringObserverBridge>(wm.getWallpaperSourceObservable());
		m_wpTypeBridge = std::make_unique<flx::ui::LvglStringObserverBridge>(wm.getWallpaperTypeObservable());
		m_wpAnimSpeedBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(wm.getAnimationSpeedObservable());
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
			wpValLabel, nullptr);

		lv_obj_t* wpTypeBtn =
			add_list_btn(m_list, LV_SYMBOL_LIST, "Wallpaper Type");
		lv_obj_set_flex_grow(lv_obj_get_child(wpTypeBtn, 1), 1);
		lv_obj_t* wpTypeLabel = lv_label_create(wpTypeBtn);

		lv_subject_add_observer_obj(
			m_wpTypeBridge->getSubject(),
			[](lv_observer_t* observer, lv_subject_t* subject) {
				lv_obj_t* label = lv_observer_get_target_obj(observer);
				if (!label) {
					return;
				}
				const char* v = static_cast<const char*>(lv_subject_get_pointer(subject));
				if (v == nullptr) {
					lv_label_set_text(label, "Static");
					return;
				}

				std::string type = v;
				if (type == "animated" || type == "gif") {
					lv_label_set_text(label, "Animated GIF");
				} else if (type == "lottie") {
					lv_label_set_text(label, "Lottie");
				} else {
					lv_label_set_text(label, "Static");
				}
			},
			wpTypeLabel, nullptr);

		lv_obj_add_event_cb(
			wpTypeBtn,
			[](lv_event_t* e) {
				auto* self = static_cast<DisplaySettings*>(lv_event_get_user_data(e));
				auto& wallpaperManager = flx::system::WallpaperManager::getInstance();
				std::string currentType = wallpaperManager.getWallpaperTypeObservable().get();
				std::string nextType = "static";
				if (currentType == "static") {
					nextType = "animated";
				} else if (currentType == "animated" || currentType == "gif") {
					nextType = "lottie";
				}

				wallpaperManager.setWallpaper(
					wallpaperManager.getWallpaperSourceObservable().get(),
					nextType);

				if (self->m_fileBrowser) {
					self->m_fileBrowser->hide();
				}
			},
			LV_EVENT_CLICKED, this);

		lv_obj_t* wpAnimSpeedBtn =
			add_list_btn(m_list, LV_SYMBOL_PLAY, "Animation Speed");
		lv_obj_t* wpAnimSlider = lv_slider_create(wpAnimSpeedBtn);
		lv_obj_set_flex_grow(wpAnimSlider, 1);
		lv_slider_set_range(wpAnimSlider, 0, 100);
		lv_slider_bind_value(wpAnimSlider, m_wpAnimSpeedBridge->getSubject());

		lv_obj_t* wpBlurBtn =
			add_list_btn(m_list, LV_SYMBOL_EDIT, "Blur");
		lv_obj_t* wpBlurSlider = lv_slider_create(wpBlurBtn);
		lv_obj_set_flex_grow(wpBlurSlider, 1);
		lv_slider_set_range(wpBlurSlider, 0, 20);
		lv_slider_set_value(wpBlurSlider, parse_int_effect("blur", 0), LV_ANIM_OFF);
		lv_obj_add_event_cb(
			wpBlurSlider,
			[](lv_event_t* e) {
				auto* sliderObj = lv_event_get_target_obj(e);
				int32_t const blur = lv_slider_get_value(sliderObj);
				auto& manager = flx::system::WallpaperManager::getInstance();
				if (blur <= 0) {
					manager.removeEffect("blur");
				} else {
					manager.applyEffect("blur", std::to_string(blur));
				}
			},
			LV_EVENT_VALUE_CHANGED, nullptr);

		lv_obj_t* wpBrightnessBtn =
			add_list_btn(m_list, LV_SYMBOL_EYE_OPEN, "Brightness");
		lv_obj_t* wpBrightnessSlider = lv_slider_create(wpBrightnessBtn);
		lv_obj_set_flex_grow(wpBrightnessSlider, 1);
		lv_slider_set_range(wpBrightnessSlider, 50, 150);
		float const brightnessValue = parse_float_effect("brightness", 1.0f);
		int32_t const brightnessPercent = static_cast<int32_t>(brightnessValue * 100.0f);
		lv_slider_set_value(wpBrightnessSlider, brightnessPercent, LV_ANIM_OFF);
		lv_obj_add_event_cb(
			wpBrightnessSlider,
			[](lv_event_t* e) {
				auto* sliderObj = lv_event_get_target_obj(e);
				int32_t const brightnessPercentLocal = lv_slider_get_value(sliderObj);
				float const brightness = static_cast<float>(brightnessPercentLocal) / 100.0f;
				auto& manager = flx::system::WallpaperManager::getInstance();
				if (brightnessPercentLocal == 100) {
					manager.removeEffect("brightness");
				} else {
					char value_buf[16];
					std::snprintf(value_buf, sizeof(value_buf), "%.2f", static_cast<double>(brightness));
					manager.applyEffect("brightness", value_buf);
				}
			},
			LV_EVENT_VALUE_CHANGED, nullptr);

		lv_obj_t* wpTransitionBtn =
			add_list_btn(m_list, LV_SYMBOL_REFRESH, "Transitions");
		lv_obj_set_flex_grow(lv_obj_get_child(wpTransitionBtn, 1), 1);
		lv_obj_t* wpTransitionSw = lv_switch_create(wpTransitionBtn);
		if (has_true_effect("fade_transition")) {
			lv_obj_add_state(wpTransitionSw, LV_STATE_CHECKED);
		}
		lv_obj_add_event_cb(
			wpTransitionSw,
			[](lv_event_t* e) {
				auto* sw = lv_event_get_target_obj(e);
				auto& manager = flx::system::WallpaperManager::getInstance();
				if (lv_obj_has_state(sw, LV_STATE_CHECKED)) {
					manager.applyEffect("fade_transition", "true");
				} else {
					manager.removeEffect("fade_transition");
				}
			},
			LV_EVENT_VALUE_CHANGED, nullptr);

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
			lv_subject_get_int(m_wpEnabledBridge->getSubject()));
		update_chooser_state(
			wpAnimSpeedBtn,
			lv_subject_get_int(m_wpEnabledBridge->getSubject()));
		update_chooser_state(
			wpBlurBtn,
			lv_subject_get_int(m_wpEnabledBridge->getSubject()));
		update_chooser_state(
			wpBrightnessBtn,
			lv_subject_get_int(m_wpEnabledBridge->getSubject()));
		update_chooser_state(
			wpTransitionBtn,
			lv_subject_get_int(m_wpEnabledBridge->getSubject()));

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
			chooseWpBtn, nullptr);

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
			wpAnimSpeedBtn, nullptr);

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
			wpBlurBtn, nullptr);

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
			wpBrightnessBtn, nullptr);

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
			wpTransitionBtn, nullptr);

		lv_subject_add_observer_obj(
			m_wpTypeBridge->getSubject(),
			[](lv_observer_t* observer, lv_subject_t* subject) {
				lv_obj_t* btn = lv_observer_get_target_obj(observer);
				if (!btn) {
					return;
				}
				const char* v = static_cast<const char*>(lv_subject_get_pointer(subject));
				std::string type = v ? v : "static";
				bool const enabled = flx::system::WallpaperManager::getInstance().getWallpaperEnabledObservable().get() != 0;
				if (!enabled || type == "static") {
					lv_obj_add_flag(btn, LV_OBJ_FLAG_HIDDEN);
				} else {
					lv_obj_remove_flag(btn, LV_OBJ_FLAG_HIDDEN);
				}
			},
			wpAnimSpeedBtn, nullptr);

		lv_obj_add_event_cb(
			chooseWpBtn,
			[](lv_event_t* e) {
				auto* self = static_cast<DisplaySettings*>(lv_event_get_user_data(e));
				auto& wallpaperManager = flx::system::WallpaperManager::getInstance();
				std::string const type = wallpaperManager.getWallpaperTypeObservable().get();
				if (!self->m_fileBrowser) {
					self->m_fileBrowser = new flx::ui::FileBrowser(self->m_parent, [self]() {
						self->m_fileBrowser->hide();
					});
				}
				if (type == "animated" || type == "gif") {
					self->m_fileBrowser->setExtensions({".gif"});
				} else if (type == "lottie") {
					self->m_fileBrowser->setExtensions({".json"});
				} else {
					self->m_fileBrowser->setExtensions({".png", ".jpg", ".jpeg", ".bmp", ".webp"});
				}
				self->m_fileBrowser->show(false, [self](const std::string& path) {
					static char path_buf[256];
					strncpy(path_buf, path.c_str(), sizeof(path_buf) - 1);
					path_buf[sizeof(path_buf) - 1] = '\0';
					lv_subject_set_pointer(
						self->m_wpPathBridge->getSubject(),
						path_buf);
					self->m_fileBrowser->hide();
				});
			},
			LV_EVENT_CLICKED, this);

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
	std::unique_ptr<flx::ui::LvglStringObserverBridge> m_wpTypeBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_wpAnimSpeedBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_transpBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_glassBridge;
};

} // namespace System::Apps::Settings
