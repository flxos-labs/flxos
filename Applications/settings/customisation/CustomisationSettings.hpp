#pragma once

#include <flx/apps/AppManager.hpp>
#include <flx/apps/Intent.hpp>
#include <flx/system/managers/ThemeManager.hpp>
#include <flx/system/managers/WallpaperManager.hpp>
#include <flx/ui/LvglObserverBridge.hpp>
#include <flx/ui/components/FileBrowser.hpp>
#include <flx/ui/theming/theme_engine/ThemeEngine.hpp>
#include <flx/ui/theming/themes/Themes.hpp>
#include <lvgl.h>
#include <memory>
#include <string>

#include "settings/SettingsPageBase.hpp"

using namespace flx::ui::common;
using namespace flx::system;

namespace System::Apps::Settings {

class CustomisationSettings : public SettingsPageBase {
public:

	using SettingsPageBase::SettingsPageBase;

protected:

	void onHide() override {
		if (m_wallpaperBrowser) {
			m_wallpaperBrowser->hide();
		}
	}

	void onDestroy() override {
		if (m_wallpaperBrowser) {
			m_wallpaperBrowser->destroy();
			m_wallpaperBrowser.reset();
		}
	}

	void createUI() override {
		auto& tm = ThemeManager::getInstance();
		auto& wm = WallpaperManager::getInstance();

		m_themeBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(tm.getThemeObservable());
		m_transpBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(tm.getTransparencyEnabledObservable());
		m_glassBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(tm.getGlassEnabledObservable());
		m_wallpaperSourceBridge = std::make_unique<flx::ui::LvglStringObserverBridge>(wm.getWallpaperSourceObservable());
		m_wallpaperBrowser = std::make_unique<flx::ui::FileBrowser>(
			m_parent,
			[this]() { showSettingsPage(); });
		m_wallpaperBrowser->setExtensions({".png", ".jpg", ".jpeg", ".bmp"});
		m_wallpaperBrowser->setInitialPath("A:/");

		m_container = create_page_container(m_parent);

		lv_obj_t* backBtn = nullptr;
		create_header(m_container, "Customisation", &backBtn);
		add_back_button_event_cb(backBtn, &m_onBack);

		m_list = create_settings_list(m_container);

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

		lv_obj_t* wallpaperEngineBtn =
			add_list_btn(m_list, LV_SYMBOL_IMAGE, "Wallpaper");
		lv_obj_set_flex_grow(lv_obj_get_child(wallpaperEngineBtn, 1), 1);
		m_wallpaperModeDropdown = lv_dropdown_create(wallpaperEngineBtn);
		lv_dropdown_set_options(m_wallpaperModeDropdown, "Default\nNone\nStatic\nDynamic");
		lv_dropdown_set_dir(m_wallpaperModeDropdown, LV_DIR_LEFT);
		lv_dropdown_set_selected(
			m_wallpaperModeDropdown,
			initialWallpaperModeIndex(wm));

		lv_obj_t* wallpaperActionBtn =
			add_list_btn(m_list, LV_SYMBOL_IMAGE, "Choose Wallpaper");
		lv_obj_add_flag(wallpaperActionBtn, LV_OBJ_FLAG_HIDDEN);
		lv_obj_set_flex_grow(lv_obj_get_child(wallpaperActionBtn, 1), 1);
		lv_obj_t* wallpaperNameLabel = lv_label_create(wallpaperActionBtn);
		lv_label_set_text(wallpaperNameLabel, wallpaperSourceToDisplayName(wm.getWallpaperSourceObservable().get()).c_str());
		lv_obj_set_style_text_opa(wallpaperNameLabel, LV_OPA_70, 0);
		lv_label_set_long_mode(wallpaperNameLabel, LV_LABEL_LONG_DOT);
		lv_obj_set_style_max_width(wallpaperNameLabel, lv_pct(45), 0);

		lv_subject_add_observer_obj(
			m_wallpaperSourceBridge->getSubject(),
			[](lv_observer_t* observer, lv_subject_t* subject) {
				lv_obj_t* label = lv_observer_get_target_obj(observer);
				if (!label) {
					return;
				}

				char const* source = static_cast<const char*>(lv_subject_get_pointer(subject));
				std::string const displayName = wallpaperSourceToDisplayName(source ? source : "");
				lv_label_set_text(label, displayName.c_str());
			},
			wallpaperNameLabel, nullptr);

		lv_obj_add_event_cb(
			m_wallpaperModeDropdown,
			[](lv_event_t* e) {
				if (lv_event_get_code(e) != LV_EVENT_VALUE_CHANGED) {
					return;
				}

				auto* actionButton = static_cast<lv_obj_t*>(lv_event_get_user_data(e));
				if (!actionButton) {
					return;
				}

				auto* dropdown = lv_event_get_target_obj(e);
				uint32_t const selectedIndex = lv_dropdown_get_selected(dropdown);
				auto& wallpaperManager = WallpaperManager::getInstance();

				if (selectedIndex == 1U) {
					wallpaperManager.getWallpaperEnabledObservable().set(0);
					wallpaperManager.getWallpaperTypeObservable().set("none");
				} else if (selectedIndex == 2U) {
					wallpaperManager.getWallpaperTypeObservable().set("static");
				} else if (selectedIndex == 3U) {
					wallpaperManager.getWallpaperTypeObservable().set("dynamic");
				}

				updateWallpaperActionButton(actionButton, selectedIndex);
			},
			LV_EVENT_VALUE_CHANGED, wallpaperActionBtn);

		lv_obj_add_event_cb(
			wallpaperActionBtn,
			[](lv_event_t* e) {
				auto* settings = static_cast<CustomisationSettings*>(lv_event_get_user_data(e));
				if (settings == nullptr || settings->m_wallpaperModeDropdown == nullptr) {
					return;
				}

				uint32_t const selectedIndex = lv_dropdown_get_selected(settings->m_wallpaperModeDropdown);
				if (selectedIndex == 2U) {
					settings->openWallpaperChooser();
					return;
				}

				if (selectedIndex == 3U) {
					flx::apps::Intent intent = flx::apps::Intent::forApp("com.flxos.wallpaper_engine");
					intent.extras.putString("route", "dynamic");
					flx::apps::AppManager::getInstance().startApp(intent);
				}
			},
			LV_EVENT_CLICKED, this);

		updateWallpaperActionButton(
			wallpaperActionBtn,
			lv_dropdown_get_selected(m_wallpaperModeDropdown));

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

		// Disable glass effect while transparency is disabled.
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

	void showSettingsPage() {
		if (m_wallpaperBrowser) {
			m_wallpaperBrowser->hide();
		}
		if (m_container) {
			lv_obj_remove_flag(m_container, LV_OBJ_FLAG_HIDDEN);
		}
	}

	void openWallpaperChooser() {
		if (!m_wallpaperBrowser) {
			return;
		}

		if (m_container) {
			lv_obj_add_flag(m_container, LV_OBJ_FLAG_HIDDEN);
		}

		m_wallpaperBrowser->show(
			false,
			[this](const std::string& selectedPath) {
				if (selectedPath.empty()) {
					return;
				}

				auto& wallpaperManager = WallpaperManager::getInstance();
				wallpaperManager.getWallpaperEnabledObservable().set(1);
				wallpaperManager.getWallpaperTypeObservable().set("static");
				wallpaperManager.setWallpaper(selectedPath);
				showSettingsPage();
			});
	}

	static uint32_t wallpaperTypeToIndex(const std::string& type) {
		if (type == "dynamic") {
			return 3U;
		}
		if (type == "static") {
			return 2U;
		}
		if (type == "none") {
			return 1U;
		}
		return 0U;
	}

	static uint32_t initialWallpaperModeIndex(WallpaperManager& wm) {
		if (wm.getWallpaperEnabledObservable().get() == 0) {
			return 1U;
		}
		return wallpaperTypeToIndex(wm.getWallpaperTypeObservable().get());
	}

	static std::string wallpaperSourceToDisplayName(const std::string& source) {
		if (source.empty()) {
			return "None";
		}

		size_t const pos = source.find_last_of('/');
		if (pos == std::string::npos || pos + 1 >= source.size()) {
			return source;
		}

		return source.substr(pos + 1);
	}

	static void updateWallpaperActionButton(lv_obj_t* actionButton, uint32_t selectedIndex) {
		if (!actionButton) {
			return;
		}

		lv_obj_t* label = lv_obj_get_child(actionButton, 1);
		if (!label) {
			return;
		}

		if (selectedIndex == 2U) {
			lv_label_set_text(label, "Choose Wallpaper");
			lv_obj_remove_flag(actionButton, LV_OBJ_FLAG_HIDDEN);
			return;
		}

		if (selectedIndex == 3U) {
			lv_label_set_text(label, "Open App");
			lv_obj_remove_flag(actionButton, LV_OBJ_FLAG_HIDDEN);
			return;
		}

		lv_obj_add_flag(actionButton, LV_OBJ_FLAG_HIDDEN);
	}

	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_themeBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_transpBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_glassBridge;
	std::unique_ptr<flx::ui::LvglStringObserverBridge> m_wallpaperSourceBridge;
	std::unique_ptr<flx::ui::FileBrowser> m_wallpaperBrowser;
	lv_obj_t* m_wallpaperModeDropdown = nullptr;
};

} // namespace System::Apps::Settings
