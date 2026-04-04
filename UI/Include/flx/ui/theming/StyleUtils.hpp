#pragma once

#include "lvgl.h"
#include <flx/ui/theming/UiThemeManager.hpp>
#include <flx/ui/theming/theme_engine/ThemeEngine.hpp>
#include <flx/ui/theming/themes/Themes.hpp>
#include <flx/ui/theming/ui_constants/UiConstants.hpp>

namespace UI::StyleUtils {

enum class ThemeColorToken {
	Primary,
	Secondary,
	Surface,
	OnPrimary,
	TextPrimary,
	TextSecondary,
	Error,
	Success,
	Warning,
	Info,
	Muted,
	Separator,
	OverlayBg,
	OverlayText,
	CardBorder,
};

enum class ThemeStatusType {
	Info,
	Success,
	Warning,
	Error,
};

static inline lv_color_t token_to_color(ThemeConfig const& cfg, ThemeColorToken token) {
	switch (token) {
		case ThemeColorToken::Primary:
			return cfg.primary;
		case ThemeColorToken::Secondary:
			return cfg.secondary;
		case ThemeColorToken::Surface:
			return cfg.surface;
		case ThemeColorToken::OnPrimary:
			return cfg.on_primary;
		case ThemeColorToken::TextPrimary:
			return cfg.text_primary;
		case ThemeColorToken::TextSecondary:
			return cfg.text_secondary;
		case ThemeColorToken::Error:
			return cfg.error;
		case ThemeColorToken::Success:
			return cfg.success;
		case ThemeColorToken::Warning:
			return cfg.warning;
		case ThemeColorToken::Info:
			return cfg.info;
		case ThemeColorToken::Muted:
			return cfg.muted;
		case ThemeColorToken::Separator:
			return cfg.separator;
		case ThemeColorToken::OverlayBg:
			return cfg.overlay_bg;
		case ThemeColorToken::OverlayText:
			return cfg.overlay_text;
		case ThemeColorToken::CardBorder:
			return cfg.card_border;
	}

	return cfg.text_primary;
}

static inline ThemeColorToken status_to_token(ThemeStatusType status) {
	switch (status) {
		case ThemeStatusType::Info:
			return ThemeColorToken::Info;
		case ThemeStatusType::Success:
			return ThemeColorToken::Success;
		case ThemeStatusType::Warning:
			return ThemeColorToken::Warning;
		case ThemeStatusType::Error:
			return ThemeColorToken::Error;
	}

	return ThemeColorToken::Info;
}

static inline void applyThemedText(lv_obj_t* obj, ThemeColorToken token) {
	using namespace flx::ui::theming;
	auto& uiTheme = UiThemeManager::getInstance();

	ThemeConfig const cfg = Themes::GetConfig(ThemeEngine::get_current_theme());
	lv_obj_set_style_text_color(obj, token_to_color(cfg, token), 0);

	lv_subject_add_observer_obj(
		uiTheme.getThemeSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			lv_obj_t* target = lv_observer_get_target_obj(observer);
			auto const token = (ThemeColorToken)(intptr_t)lv_observer_get_user_data(observer);
			ThemeType theme = (ThemeType)lv_subject_get_int(subject);
			ThemeConfig const cfgInner = Themes::GetConfig(theme);
			lv_obj_set_style_text_color(target, token_to_color(cfgInner, token), 0);
		},
		obj, (void*)(intptr_t)token);
}

static inline void applyThemedBg(lv_obj_t* obj, ThemeColorToken token, lv_opa_t opa = UiConstants::OPA_COVER) {
	using namespace flx::ui::theming;
	auto& uiTheme = UiThemeManager::getInstance();

	ThemeConfig const cfg = Themes::GetConfig(ThemeEngine::get_current_theme());
	lv_obj_set_style_bg_color(obj, token_to_color(cfg, token), 0);
	lv_obj_set_style_bg_opa(obj, opa, 0);

	lv_subject_add_observer_obj(
		uiTheme.getThemeSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			lv_obj_t* target = lv_observer_get_target_obj(observer);
			auto const token = (ThemeColorToken)(intptr_t)lv_observer_get_user_data(observer);
			ThemeType theme = (ThemeType)lv_subject_get_int(subject);
			ThemeConfig const cfgInner = Themes::GetConfig(theme);
			lv_obj_set_style_bg_color(target, token_to_color(cfgInner, token), 0);
		},
		obj, (void*)(intptr_t)token);
}

static inline void applyThemedBorder(
	lv_obj_t* obj,
	ThemeColorToken token,
	int32_t width = UiConstants::BORDER_THIN,
	lv_opa_t opa = UiConstants::OPA_COVER) {
	using namespace flx::ui::theming;
	auto& uiTheme = UiThemeManager::getInstance();

	ThemeConfig const cfg = Themes::GetConfig(ThemeEngine::get_current_theme());
	lv_obj_set_style_border_width(obj, lv_dpx(width), 0);
	lv_obj_set_style_border_opa(obj, opa, 0);
	lv_obj_set_style_border_color(obj, token_to_color(cfg, token), 0);

	lv_subject_add_observer_obj(
		uiTheme.getThemeSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			lv_obj_t* target = lv_observer_get_target_obj(observer);
			auto const token = (ThemeColorToken)(intptr_t)lv_observer_get_user_data(observer);
			ThemeType theme = (ThemeType)lv_subject_get_int(subject);
			ThemeConfig const cfgInner = Themes::GetConfig(theme);
			lv_obj_set_style_border_color(target, token_to_color(cfgInner, token), 0);
		},
		obj, (void*)(intptr_t)token);
}

static inline void applyThemedStatusColor(lv_obj_t* obj, ThemeStatusType status) {
	applyThemedText(obj, status_to_token(status));
}

static inline void applyThemedSeparator(lv_obj_t* obj, lv_opa_t opa = UiConstants::OPA_40) {
	lv_obj_set_size(obj, lv_pct(100), lv_dpx(UiConstants::BORDER_THIN));
	applyThemedBg(obj, ThemeColorToken::Separator, opa);
	// Separator is a visual rule only, no border/shadow needed.
	lv_obj_set_style_border_width(obj, 0, 0);
	lv_obj_set_style_shadow_width(obj, 0, 0);
}

static inline void apply_glass(lv_obj_t* obj, int32_t blur) {
	using namespace flx::ui::theming;
	auto& uiTheme = UiThemeManager::getInstance();

	ThemeConfig const cfg = Themes::GetConfig(ThemeEngine::get_current_theme());
	lv_obj_set_style_bg_color(obj, cfg.surface, 0);
	lv_obj_set_style_bg_opa(obj, UiConstants::OPA_GLASS_BG, 0);
	lv_obj_set_style_text_color(obj, cfg.text_primary, 0);

	// Add observer for Theme changes
	lv_subject_add_observer_obj(
		uiTheme.getThemeSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			lv_obj_t* target = lv_observer_get_target_obj(observer);
			ThemeType theme = (ThemeType)lv_subject_get_int(subject);
			ThemeConfig cfg = Themes::GetConfig(theme);
			lv_obj_set_style_bg_color(target, cfg.surface, 0);
			lv_obj_set_style_text_color(target, cfg.text_primary, 0);
		},
		obj, nullptr);

	// Add observer for Glass Enabled
	lv_subject_add_observer_obj(
		uiTheme.getGlassEnabledSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			lv_obj_t* target = lv_observer_get_target_obj(observer);
			auto const B = (intptr_t)lv_observer_get_user_data(observer);
			bool const GLASS_ENABLED = lv_subject_get_int(subject);

			auto& uiThemeInner = UiThemeManager::getInstance();
			bool const TRANSP_ENABLED = lv_subject_get_int(uiThemeInner.getTransparencyEnabledSubject());

			if (GLASS_ENABLED && TRANSP_ENABLED) {
				lv_obj_set_style_blur_backdrop(target, true, 0);
				lv_obj_set_style_blur_radius(target, B, 0);
			} else {
				lv_obj_set_style_blur_backdrop(target, false, 0);
				lv_obj_set_style_blur_radius(target, 0, 0);
			}
		},
		obj, (void*)(intptr_t)blur);

	// Add observer for Transparency Enabled
	lv_subject_add_observer_obj(
		uiTheme.getTransparencyEnabledSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			lv_obj_t* target = lv_observer_get_target_obj(observer);
			auto const B = (intptr_t)lv_observer_get_user_data(observer);

			bool const TRANSP_ENABLED = lv_subject_get_int(subject);
			if (TRANSP_ENABLED) {
				lv_obj_set_style_bg_opa(target, UiConstants::OPA_GLASS_BG, 0);

				// Re-check glass status to re-enable blur if needed
				auto& uiThemeInner = UiThemeManager::getInstance();
				bool const GLASS_ENABLED = lv_subject_get_int(uiThemeInner.getGlassEnabledSubject());

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
		obj, (void*)(intptr_t)blur);
}

} // namespace UI::StyleUtils
