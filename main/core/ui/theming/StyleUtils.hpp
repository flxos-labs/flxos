#pragma once

#include "core/system/theme/ThemeManager.hpp"
#include "lvgl.h"
#include "theme_engine/ThemeEngine.hpp"
#include "themes/Themes.hpp"
#include "ui_constants/UiConstants.hpp"

namespace UI::StyleUtils {

static inline void apply_glass(lv_obj_t* obj, int32_t blur) {
	ThemeConfig const cfg = Themes::GetConfig(ThemeEngine::get_current_theme());
	lv_obj_set_style_bg_color(obj, cfg.surface, 0);
	lv_obj_set_style_bg_opa(obj, UiConstants::OPA_GLASS_BG, 0);
	lv_obj_set_style_text_color(obj, cfg.text_primary, 0);
	lv_obj_set_style_image_recolor(obj, cfg.text_primary, 0);
	lv_obj_set_style_image_recolor_opa(obj, UiConstants::OPA_COVER, 0);

	// Add observer for Theme changes
	lv_subject_add_observer_obj(
		&System::ThemeManager::getInstance().getThemeSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			lv_obj_t* target = lv_observer_get_target_obj(observer);
			ThemeType theme = (ThemeType)lv_subject_get_int(subject);
			ThemeConfig cfg = Themes::GetConfig(theme);
			lv_obj_set_style_bg_color(target, cfg.surface, 0);
			lv_obj_set_style_text_color(target, cfg.text_primary, 0);
			lv_obj_set_style_image_recolor(target, cfg.text_primary, 0);
		},
		obj, nullptr
	);

	// Add observer for Glass Enabled
	lv_subject_add_observer_obj(
		&System::ThemeManager::getInstance().getGlassEnabledSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			lv_obj_t* target = lv_observer_get_target_obj(observer);
			auto const B = (intptr_t)lv_observer_get_user_data(observer);
			bool const GLASS_ENABLED = lv_subject_get_int(subject);
			bool const TRANSP_ENABLED = lv_subject_get_int(&System::ThemeManager::getInstance().getTransparencyEnabledSubject());

			if (GLASS_ENABLED && TRANSP_ENABLED) {
				lv_obj_set_style_blur_backdrop(target, true, 0);
				lv_obj_set_style_blur_radius(target, B, 0);
			} else {
				lv_obj_set_style_blur_backdrop(target, false, 0);
				lv_obj_set_style_blur_radius(target, 0, 0);
			}
		},
		obj, (void*)(intptr_t)blur
	);

	// Add observer for Transparency Enabled
	lv_subject_add_observer_obj(
		&System::ThemeManager::getInstance().getTransparencyEnabledSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			lv_obj_t* target = lv_observer_get_target_obj(observer);
			auto const B = (intptr_t)lv_observer_get_user_data(observer);

			bool const TRANSP_ENABLED = lv_subject_get_int(subject);
			if (TRANSP_ENABLED) {
				lv_obj_set_style_bg_opa(target, UiConstants::OPA_GLASS_BG, 0);

				// Re-check glass status to re-enable blur if needed
				bool const GLASS_ENABLED = lv_subject_get_int(&System::ThemeManager::getInstance().getGlassEnabledSubject());

				if (GLASS_ENABLED) {
					lv_obj_set_style_blur_backdrop(target, true, 0);
					lv_obj_set_style_blur_radius(target, B, 0);
				}
			} else {
				lv_obj_set_style_bg_opa(target, UiConstants::OPA_COVER, 0);
				lv_obj_set_style_blur_backdrop(target, false, 0);
				lv_obj_set_style_blur_radius(target, 0, 0);
			}
		},
		obj, (void*)(intptr_t)blur
	);
}

} // namespace UI::StyleUtils
