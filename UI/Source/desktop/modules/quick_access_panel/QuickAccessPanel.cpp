#include <flx/ui/desktop/modules/quick_access_panel/QuickAccessPanel.hpp>
#include <flx/ui/theming/StyleUtils.hpp>
#include <flx/ui/theming/layout_constants/LayoutConstants.hpp>
#include <flx/ui/theming/theme_engine/ThemeEngine.hpp>
#include <flx/ui/theming/ui_constants/UiConstants.hpp>

#include "core/lv_obj.h"
#include "core/lv_obj_pos.h"
#include "core/lv_obj_style.h"
#include "core/lv_obj_style_gen.h"
#include "core/lv_observer.h"
#include "display/lv_display.h"
#include "draw/lv_draw_rect.h"
#include "flx/ui/desktop/window_manager/WindowManager.hpp"
#include "flx/ui/theming/themes/Themes.hpp"
#include "font/lv_symbol_def.h"
#include "layouts/flex/lv_flex.h"
#include "misc/lv_area.h"
#include "misc/lv_event.h"
#include "misc/lv_log.h"
#include "misc/lv_text.h"
#include "misc/lv_timer.h"
#include "misc/lv_types.h"
#include "widgets/button/lv_button.h"
#include "widgets/image/lv_image.h"
#include "widgets/label/lv_label.h"
#include "widgets/slider/lv_slider.h"
#include <cstdint>
#include <flx/apps/AppManager.hpp>
#include <flx/apps/Intent.hpp>
#include <flx/core/Logger.hpp>
#include <flx/system/managers/DisplayManager.hpp>
#include <flx/system/managers/ThemeManager.hpp>
#include <flx/system/services/ScreenshotService.hpp>
#include <flx/ui/desktop/modules/dock/Dock.hpp>
#include <flx/ui/managers/FocusManager.hpp>
#include <flx/ui/theming/UiThemeManager.hpp>

namespace UI::Modules {

QuickAccessPanel::QuickAccessPanel(lv_obj_t* parent, lv_obj_t* dock)
	: m_parent(parent), m_dock(dock) {

	if (!m_parent || !m_dock) {
		Log::error("QuickAccessPanel", "Parent or Dock is NULL");
		return;
	}
	create();
}

QuickAccessPanel::~QuickAccessPanel() {
	if (m_panel && lv_obj_is_valid(m_panel)) {
		lv_obj_del(m_panel);
		m_panel = nullptr;
	}
}

void QuickAccessPanel::create() {
	if (!setupPanel()) return;

	buildHeader();
	buildToggles();
	buildBrightnessSlider();
}

bool QuickAccessPanel::setupPanel() {
	m_panel = lv_obj_create(m_parent);
	if (!m_panel) {
		LV_LOG_ERROR("QuickAccessPanel: Failed to create main panel");
		return false;
	}

	lv_obj_set_size(m_panel, lv_pct(LayoutConstants::PANEL_WIDTH_PCT), lv_pct(LayoutConstants::PANEL_HEIGHT_PCT));
	lv_obj_set_style_pad_all(m_panel, 0, 0);
	lv_obj_set_style_radius(m_panel, lv_dpx(UiConstants::RADIUS_LARGE), 0);
	lv_obj_set_style_border_width(m_panel, 0, 0);
	lv_obj_add_flag(m_panel, LV_OBJ_FLAG_FLOATING);
	lv_obj_add_flag(m_panel, LV_OBJ_FLAG_HIDDEN);

	UI::StyleUtils::apply_glass(m_panel, lv_dpx(UiConstants::GLASS_BLUR_SMALL));

	lv_obj_align_to(m_panel, m_dock, LV_ALIGN_OUT_TOP_RIGHT, 0, -lv_dpx(UiConstants::OFFSET_TINY));
	lv_obj_set_flex_flow(m_panel, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(m_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	return true;
}

void QuickAccessPanel::buildHeader() {
	lv_obj_t* header_cont = lv_obj_create(m_panel);
	if (!header_cont) return;

	lv_obj_remove_style_all(header_cont);
	lv_obj_set_size(header_cont, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(header_cont, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(header_cont, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_left(header_cont, lv_dpx(UiConstants::PAD_SMALL), 0);
	lv_obj_set_style_pad_right(header_cont, lv_dpx(UiConstants::PAD_SMALL), 0);
	lv_obj_set_style_pad_top(header_cont, lv_dpx(UiConstants::PAD_SMALL), 0);

	lv_obj_t* label = lv_label_create(header_cont);
	if (label) {
		lv_label_set_text(label, "Quick Access");
		lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);
	}

	lv_obj_t* settings_btn = lv_button_create(header_cont);
	if (!settings_btn) return;

	lv_obj_set_size(settings_btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_style_bg_opa(settings_btn, LV_OPA_TRANSP, 0);
	lv_obj_set_style_shadow_width(settings_btn, 0, 0);
	lv_obj_set_style_pad_all(settings_btn, lv_dpx(UiConstants::PAD_SMALL), 0);

	lv_obj_t* settings_icon = lv_image_create(settings_btn);
	if (settings_icon) {
		lv_image_set_src(settings_icon, LV_SYMBOL_SETTINGS);
		lv_obj_center(settings_icon);
	}

	lv_obj_add_event_cb(settings_btn, [](lv_event_t* e) {
        flx::ui::FocusManager::getInstance().dismissAllPanels();
        flx::apps::AppManager::getInstance().startApp(
            flx::apps::Intent::forApp("com.flxos.settings")
        ); }, LV_EVENT_CLICKED, nullptr);

	lv_obj_set_style_text_color(settings_btn, Themes::GetConfig(ThemeEngine::get_current_theme()).text_primary, 0);

	auto& uiTheme = flx::ui::theming::UiThemeManager::getInstance();
	lv_subject_add_observer_obj(
		uiTheme.getThemeSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			lv_obj_t* btn = lv_observer_get_target_obj(observer);
			if (btn && subject) {
				ThemeType theme = (ThemeType)lv_subject_get_int(subject);
				ThemeConfig cfg = Themes::GetConfig(theme);
				lv_obj_set_style_text_color(btn, cfg.text_primary, 0);
			}
		},
		settings_btn, nullptr
	);
}

void QuickAccessPanel::buildToggles() {
	lv_obj_t* toggles_cont = lv_obj_create(m_panel);
	if (!toggles_cont) return;

	lv_obj_remove_style_all(toggles_cont);
	lv_obj_set_size(toggles_cont, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(toggles_cont, LV_FLEX_FLOW_ROW_WRAP);
	lv_obj_set_style_pad_gap(toggles_cont, lv_dpx(UiConstants::PAD_SMALL), 0);
	lv_obj_set_flex_align(toggles_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	auto& uiTheme = flx::ui::theming::UiThemeManager::getInstance();

	// --- Theme Toggle ---
	{
		lv_obj_t* theme_cont = lv_obj_create(toggles_cont);
		if (theme_cont) {
			lv_obj_remove_style_all(theme_cont);
			lv_obj_set_size(theme_cont, lv_pct(30), LV_SIZE_CONTENT);
			lv_obj_set_flex_flow(theme_cont, LV_FLEX_FLOW_COLUMN);
			lv_obj_set_flex_align(theme_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

			lv_obj_t* theme_btn = lv_button_create(theme_cont);
			if (theme_btn) {
				lv_obj_set_size(theme_btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
				lv_obj_set_style_radius(theme_btn, LV_RADIUS_CIRCLE, 0);
				lv_obj_add_flag(theme_btn, LV_OBJ_FLAG_CHECKABLE);

				lv_obj_t* theme_icon = lv_label_create(theme_btn);
				lv_label_set_text(theme_icon, LV_SYMBOL_EDIT);
				lv_obj_center(theme_icon);

				lv_obj_bind_checked(theme_btn, uiTheme.getThemeSubject());
			}

			m_themeLabel = lv_label_create(theme_cont);
			if (m_themeLabel) {
				lv_obj_set_width(m_themeLabel, lv_pct(100));
				lv_label_set_long_mode(m_themeLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
				lv_obj_set_style_text_align(m_themeLabel, LV_TEXT_ALIGN_CENTER, 0);

				lv_subject_add_observer_obj(
					uiTheme.getThemeSubject(),
					[](lv_observer_t* observer, lv_subject_t* subject) {
						lv_obj_t* label = lv_observer_get_target_obj(observer);
						if (label && subject) {
							ThemeType theme = (ThemeType)lv_subject_get_int(subject);
							lv_label_set_text(label, Themes::ToString(theme));
						}
					},
					m_themeLabel, nullptr
				);
			}
		}
	}

	// --- Rotation Toggle ---
	{
		lv_obj_t* rot_cont = lv_obj_create(toggles_cont);
		if (rot_cont) {
			lv_obj_remove_style_all(rot_cont);
			lv_obj_set_size(rot_cont, lv_pct(30), LV_SIZE_CONTENT);
			lv_obj_set_flex_flow(rot_cont, LV_FLEX_FLOW_COLUMN);
			lv_obj_set_flex_align(rot_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

			m_rotationBridge.reset(new flx::ui::LvglObserverBridge<int32_t>(flx::system::DisplayManager::getInstance().getRotationObservable()));

			lv_obj_t* rot_btn = lv_button_create(rot_cont);
			if (rot_btn) {
				lv_obj_set_size(rot_btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
				lv_obj_set_style_radius(rot_btn, LV_RADIUS_CIRCLE, 0);
				lv_obj_set_style_bg_color(rot_btn, lv_palette_main(LV_PALETTE_BLUE), 0);

				lv_obj_t* rot_icon = lv_label_create(rot_btn);
				lv_label_set_text(rot_icon, LV_SYMBOL_REFRESH);
				lv_obj_center(rot_icon);

				lv_subject_increment_dsc_t* rot_dsc = lv_obj_add_subject_increment_event(
					rot_btn, m_rotationBridge->getSubject(), LV_EVENT_CLICKED, 90
				);

				if (rot_dsc) {
					lv_obj_set_subject_increment_event_min_value(rot_btn, rot_dsc, 0);
					lv_obj_set_subject_increment_event_max_value(rot_btn, rot_dsc, 270);
					lv_obj_set_subject_increment_event_rollover(rot_btn, rot_dsc, true);
				}
			}

			lv_obj_t* rot_label = lv_label_create(rot_cont);
			if (rot_label) {
				lv_obj_set_width(rot_label, lv_pct(100));
				lv_label_set_long_mode(rot_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
				lv_obj_set_style_text_align(rot_label, LV_TEXT_ALIGN_CENTER, 0);
				lv_label_bind_text(rot_label, m_rotationBridge->getSubject(), "%d°");
			}
		}
	}

	// --- Screenshot Toggle ---
	{
		lv_obj_t* shot_cont = lv_obj_create(toggles_cont);
		if (shot_cont) {
			lv_obj_remove_style_all(shot_cont);
			lv_obj_set_size(shot_cont, lv_pct(30), LV_SIZE_CONTENT);
			lv_obj_set_flex_flow(shot_cont, LV_FLEX_FLOW_COLUMN);
			lv_obj_set_flex_align(shot_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

			lv_obj_t* shot_btn = lv_button_create(shot_cont);
			if (shot_btn) {
				lv_obj_set_size(shot_btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
				lv_obj_set_style_radius(shot_btn, LV_RADIUS_CIRCLE, 0);
				lv_obj_t* shot_icon = lv_image_create(shot_btn);
				if (shot_icon) {
					lv_image_set_src(shot_icon, LV_SYMBOL_CUT);
					lv_obj_center(shot_icon);
				}

				lv_obj_add_event_cb(shot_btn, [](lv_event_t* e) {
                    flx::ui::FocusManager::getInstance().dismissAllPanels();

                    uint32_t delaySec = flx::services::ScreenshotService::getInstance().getDefaultDelay();
                    if (delaySec < 1) delaySec = 1;

                    flx::services::ScreenshotService::getInstance().scheduleCapture(delaySec); }, LV_EVENT_CLICKED, nullptr);
			}

			lv_obj_t* shot_label = lv_label_create(shot_cont);
			if (shot_label) {
				lv_obj_set_width(shot_label, lv_pct(100));
				lv_label_set_long_mode(shot_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
				lv_obj_set_style_text_align(shot_label, LV_TEXT_ALIGN_CENTER, 0);
				lv_label_set_text(shot_label, "Screenshot");
			}
		}
	}
}

void QuickAccessPanel::buildBrightnessSlider() {
	lv_obj_t* slider_cont = lv_obj_create(m_panel);
	if (!slider_cont) return;

	lv_obj_remove_style_all(slider_cont);
	lv_obj_set_size(slider_cont, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(slider_cont, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(slider_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_t* icon = lv_image_create(slider_cont);
	if (icon) {
		lv_image_set_src(icon, LV_SYMBOL_EYE_OPEN);
	}

	m_brightnessBridge.reset(new flx::ui::LvglObserverBridge<int32_t>(flx::system::DisplayManager::getInstance().getBrightnessObservable()));
	lv_obj_t* slider = lv_slider_create(slider_cont);
	if (slider) {
		lv_obj_set_flex_grow(slider, 1);
		lv_slider_set_range(slider, 0, 255);
		lv_obj_set_style_anim_duration(slider, 200, 0);
		lv_slider_bind_value(slider, m_brightnessBridge->getSubject());
	}
}

} // namespace UI::Modules
