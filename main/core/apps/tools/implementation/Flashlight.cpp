#include "Flashlight.hpp"
#include "../../ui/theming/ui_constants/UiConstants.hpp"
#include "core/apps/settings/SettingsCommon.hpp"

namespace System::Apps::Tools {

void Flashlight::createView(lv_obj_t* parent, std::function<void()> onBack) {
	m_view = Settings::create_page_container(parent);

	lv_obj_t* backBtn = nullptr;
	Settings::create_header(m_view, "Flashlight", &backBtn);

	m_onBack = onBack;
	Settings::add_back_button_event_cb(backBtn, &m_onBack);

	m_flashlightContainer = lv_obj_create(m_view);
	lv_obj_set_size(m_flashlightContainer, lv_pct(100), lv_pct(100));
	lv_obj_set_flex_grow(m_flashlightContainer, 1);
	lv_obj_set_style_bg_color(m_flashlightContainer, lv_color_hex(0x2d2d2d), 0);
	lv_obj_set_style_border_width(m_flashlightContainer, 0, 0);
	lv_obj_set_flex_flow(m_flashlightContainer, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(m_flashlightContainer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_t* icon = lv_label_create(m_flashlightContainer);
	lv_label_set_text(icon, LV_SYMBOL_EYE_OPEN);

	lv_obj_t* hint = lv_label_create(m_flashlightContainer);
	lv_label_set_text(hint, "Tap to toggle");
	lv_obj_set_style_margin_top(hint, lv_dpx(UiConstants::PAD_LARGE), 0);

	lv_obj_add_event_cb(m_flashlightContainer, [](lv_event_t* e) {
        auto* app = static_cast<Flashlight*>(lv_event_get_user_data(e));
        app->m_flashlightOn = !app->m_flashlightOn;

        if (app->m_flashlightOn) {
            lv_obj_set_style_bg_color(app->m_flashlightContainer, lv_color_white(), 0);
            lv_obj_set_style_text_color(lv_obj_get_child(app->m_flashlightContainer, 0), lv_color_black(), 0);
            lv_obj_set_style_text_color(lv_obj_get_child(app->m_flashlightContainer, 1), lv_color_black(), 0);
        } else {
            lv_obj_set_style_bg_color(app->m_flashlightContainer, lv_color_hex(0x2d2d2d), 0);
            lv_obj_set_style_text_color(lv_obj_get_child(app->m_flashlightContainer, 0), lv_color_white(), 0);
            lv_obj_set_style_text_color(lv_obj_get_child(app->m_flashlightContainer, 1), lv_color_white(), 0);
        } }, LV_EVENT_CLICKED, this);
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
	m_flashlightOn = false;
}

} // namespace System::Apps::Tools
