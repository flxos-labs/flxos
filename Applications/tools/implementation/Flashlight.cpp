#include "Flashlight.hpp"
#include <flx/ui/common/SettingsCommon.hpp>
#include <flx/ui/theming/UiThemeManager.hpp>
#include <flx/ui/theming/theme_engine/ThemeEngine.hpp>
#include <flx/ui/theming/themes/Themes.hpp>
#include <flx/ui/theming/ui_constants/UiConstants.hpp>

using namespace flx::ui::common;

namespace System::Apps::Tools {

void Flashlight::createView(lv_obj_t* parent, std::function<void()> onBack) {
	m_view = create_page_container(parent);

	lv_obj_t* backBtn = nullptr;
	create_header(m_view, "Flashlight", &backBtn);

	m_onBack = onBack;
	add_back_button_event_cb(backBtn, &m_onBack);

	m_flashlightContainer = lv_obj_create(m_view);
	lv_obj_set_size(m_flashlightContainer, lv_pct(100), lv_pct(100));
	lv_obj_set_flex_grow(m_flashlightContainer, 1);
	lv_obj_set_style_border_width(m_flashlightContainer, 0, 0);
	lv_obj_set_flex_flow(m_flashlightContainer, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(m_flashlightContainer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	m_flashlightIcon = lv_label_create(m_flashlightContainer);
	lv_label_set_text(m_flashlightIcon, LV_SYMBOL_EYE_OPEN);

	m_flashlightHint = lv_label_create(m_flashlightContainer);
	lv_label_set_text(m_flashlightHint, "Tap to toggle");
	lv_obj_set_style_margin_top(m_flashlightHint, lv_dpx(UiConstants::PAD_LARGE), 0);

	applyThemeStyles();

	auto& uiTheme = flx::ui::theming::UiThemeManager::getInstance();
	lv_subject_add_observer_obj(
		uiTheme.getThemeSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			LV_UNUSED(subject);
			auto* app = static_cast<Flashlight*>(lv_observer_get_user_data(observer));
			if (app) {
				app->applyThemeStyles();
			}
		},
		m_view, this);

	lv_obj_add_event_cb(m_flashlightContainer, [](lv_event_t* e) {
        auto* app = static_cast<Flashlight*>(lv_event_get_user_data(e));
        app->m_flashlightOn = !app->m_flashlightOn;
		app->applyThemeStyles(); }, LV_EVENT_CLICKED, this);
}

void Flashlight::show() {
	if (m_view) lv_obj_remove_flag(m_view, LV_OBJ_FLAG_HIDDEN);
}

void Flashlight::hide() {
	if (m_view) lv_obj_add_flag(m_view, LV_OBJ_FLAG_HIDDEN);
}

void Flashlight::destroy() {
	if (m_view) {
		lv_obj_del(m_view);
		m_view = nullptr;
	}
	m_flashlightContainer = nullptr;
	m_flashlightIcon = nullptr;
	m_flashlightHint = nullptr;
	m_flashlightOn = false;
}

void Flashlight::applyThemeStyles() {
	if (!m_flashlightContainer || !m_flashlightIcon || !m_flashlightHint) {
		return;
	}

	if (m_flashlightOn) {
		lv_obj_set_style_bg_color(m_flashlightContainer, lv_color_white(), 0);
		lv_obj_set_style_text_color(m_flashlightIcon, lv_color_black(), 0);
		lv_obj_set_style_text_color(m_flashlightHint, lv_color_black(), 0);
		return;
	}

	ThemeConfig const theme = Themes::GetConfig(ThemeEngine::get_current_theme());
	// In light themes, surface can be white and visually match the ON state.
	// Use muted for OFF so toggle state remains obvious.
	lv_color_t const offBg = theme.dark ? theme.surface : theme.muted;
	lv_obj_set_style_bg_color(m_flashlightContainer, offBg, 0);
	lv_obj_set_style_text_color(m_flashlightIcon, theme.text_primary, 0);
	lv_obj_set_style_text_color(m_flashlightHint, theme.text_primary, 0);
}

} // namespace System::Apps::Tools
