#include "DisplayTester.hpp"
#include <cstdint>
#include <flx/ui/common/SettingsCommon.hpp>
#include <flx/ui/theming/layout_constants/LayoutConstants.hpp>
#include <flx/ui/theming/ui_constants/UiConstants.hpp>

namespace System::Apps::Tools {

using namespace flx::ui::common;

void DisplayTester::createView(lv_obj_t* parent, std::function<void()> onBack) {
	m_view = create_page_container(parent);

	lv_obj_t* backBtn = nullptr;
	create_header(m_view, "Display Tester", &backBtn);

	m_onBack = onBack;
	add_back_button_event_cb(backBtn, &m_onBack);

	// Color display
	m_rgbDisplay = lv_obj_create(m_view);
	lv_obj_set_width(m_rgbDisplay, lv_pct(100));
	lv_obj_set_flex_grow(m_rgbDisplay, 1);
	lv_obj_set_style_bg_color(m_rgbDisplay, lv_color_hex(0xFF0000), 0);
	lv_obj_set_style_border_width(m_rgbDisplay, 0, 0);
	lv_obj_set_flex_flow(m_rgbDisplay, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(m_rgbDisplay, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_clear_flag(m_rgbDisplay, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t* colorLabel = lv_label_create(m_rgbDisplay);
	lv_label_set_text(colorLabel, "RED");
	lv_obj_set_style_text_color(colorLabel, lv_color_white(), 0);

	// Color buttons
	lv_obj_t* btnRow = lv_obj_create(m_view);
	lv_obj_set_size(btnRow, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(btnRow, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(btnRow, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_bg_opa(btnRow, 0, 0);
	lv_obj_set_style_border_width(btnRow, 0, 0);
	lv_obj_set_style_pad_all(btnRow, lv_dpx(UiConstants::PAD_DEFAULT), 0);

	struct ColorDef {
		uint32_t color;
		const char* name;
		lv_color_t textColor;
	};

	static const ColorDef colors[] = {
		{0xFF0000, "RED", lv_color_white()},
		{0x00FF00, "GREEN", lv_color_black()},
		{0x0000FF, "BLUE", lv_color_white()},
		{0xFFFFFF, "WHITE", lv_color_black()},
		{0x000000, "BLACK", lv_color_white()}
	};

	for (int i = 0; i < 5; i++) {
		lv_obj_t* btn = lv_button_create(btnRow);
		lv_obj_set_size(btn, lv_pct(18), lv_dpx(LayoutConstants::SIZE_TOUCH_TARGET));
		lv_obj_set_style_bg_color(btn, lv_color_hex(colors[i].color), 0);
		if (colors[i].color == 0x000000) {
			lv_obj_set_style_border_color(btn, lv_color_white(), 0);
			lv_obj_set_style_border_width(btn, UiConstants::BORDER_THIN, 0);
		}

		lv_obj_set_user_data(btn, (void*)(uintptr_t)i);

		lv_obj_add_event_cb(btn, [](lv_event_t* e) {
            auto* app = static_cast<DisplayTester*>(lv_event_get_user_data(e));
            auto* target = static_cast<lv_obj_t*>(lv_event_get_target(e));
            int idx = (int)(uintptr_t)lv_obj_get_user_data(target);

            lv_obj_set_style_bg_color(app->m_rgbDisplay, lv_color_hex(colors[idx].color), 0);
            lv_obj_t* label = lv_obj_get_child(app->m_rgbDisplay, 0);
            lv_label_set_text(label, colors[idx].name);
            lv_obj_set_style_text_color(label, colors[idx].textColor, 0); }, LV_EVENT_CLICKED, this);
	}
}

void DisplayTester::show() {
	if (m_view) lv_obj_remove_flag(m_view, LV_OBJ_FLAG_HIDDEN);
}

void DisplayTester::hide() {
	if (m_view) lv_obj_add_flag(m_view, LV_OBJ_FLAG_HIDDEN);
}

void DisplayTester::destroy() {
	if (m_view) {
		lv_obj_del(m_view);
		m_view = nullptr;
	}
	m_rgbDisplay = nullptr;
}

} // namespace System::Apps::Tools
