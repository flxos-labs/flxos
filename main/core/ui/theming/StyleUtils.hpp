#pragma once

#include "ui_constants/UiConstants.hpp"
#include "core/system/theme/ThemeManager.hpp"
#include "lvgl.h"

namespace UI {
namespace StyleUtils {

static inline void apply_glass(lv_obj_t* obj, int32_t blur) {
	lv_obj_set_style_bg_opa(obj, UiConstants::OPA_GLASS_BG, 0);

	// Add observer for Glass Enabled
	lv_subject_add_observer_obj(
		&System::ThemeManager::getInstance().getGlassEnabledSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			lv_obj_t* target = lv_observer_get_target_obj(observer);
			int32_t b = (intptr_t)lv_observer_get_user_data(observer);
			bool glass_enabled = lv_subject_get_int(subject);
			bool transp_enabled = lv_subject_get_int(&System::ThemeManager::getInstance().getTransparencyEnabledSubject());

			if (glass_enabled && transp_enabled) {
				lv_obj_set_style_blur_backdrop(target, true, 0);
				lv_obj_set_style_blur_radius(target, b, 0);
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
			int32_t b = (intptr_t)lv_observer_get_user_data(observer);

			bool transp_enabled = lv_subject_get_int(subject);
			if (transp_enabled) {
				lv_obj_set_style_bg_opa(target, UiConstants::OPA_GLASS_BG, 0);

				// Re-check glass status to re-enable blur if needed
				bool glass_enabled = lv_subject_get_int(&System::ThemeManager::getInstance().getGlassEnabledSubject());

				if (glass_enabled) {
					lv_obj_set_style_blur_backdrop(target, true, 0);
					lv_obj_set_style_blur_radius(target, b, 0);
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

} // namespace StyleUtils
} // namespace UI
