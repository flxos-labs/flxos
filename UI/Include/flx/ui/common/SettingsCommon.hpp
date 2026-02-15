#pragma once

#include "lvgl.h"
#include <flx/ui/theming/ui_constants/UiConstants.hpp>
#include <functional>

namespace flx::ui::common {

inline lv_obj_t* add_list_btn(lv_obj_t* list, const char* symbol, const char* text) {
	lv_obj_t* btn = lv_list_add_button(list, nullptr, text);
	lv_obj_t* img = lv_image_create(btn);
	lv_image_set_src(img, symbol);
	lv_obj_move_to_index(img, 0);
	return btn;
}

inline lv_obj_t* create_page_container(lv_obj_t* parent) {
	lv_obj_t* cont = lv_obj_create(parent);
	lv_obj_set_size(cont, lv_pct(100), lv_pct(100));
	lv_obj_set_style_pad_all(cont, 0, 0);
	lv_obj_set_style_border_width(cont, 0, 0);
	lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_gap(cont, 0, 0);
	return cont;
}

inline lv_obj_t* create_header(lv_obj_t* parent, const char* title_text, lv_obj_t** back_btn_out = nullptr) {
	lv_obj_t* header = lv_obj_create(parent);
	lv_obj_set_size(header, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_style_max_height(header, lv_dpx(UiConstants::SIZE_TAB_BAR), 0);
	lv_obj_set_style_pad_all(header, 0, 0);
	lv_obj_set_style_pad_gap(header, lv_dpx(UiConstants::PAD_SMALL), 0);
	lv_obj_set_style_border_width(header, 0, 0);
	lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, 0);
	lv_obj_set_style_border_width(header, lv_dpx(UiConstants::BORDER_THIN), 0);
	lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(header, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_t* backBtn = lv_button_create(header);
	lv_obj_t* backLabel = lv_image_create(backBtn);
	lv_image_set_src(backLabel, LV_SYMBOL_LEFT);
	if (back_btn_out) {
		*back_btn_out = backBtn;
	}

	lv_obj_t* title = lv_label_create(header);
	lv_label_set_text(title, title_text);

	return header;
}

inline lv_obj_t* create_settings_list(lv_obj_t* parent) {
	lv_obj_t* list = lv_list_create(parent);
	lv_obj_set_width(list, lv_pct(100));
	lv_obj_set_flex_grow(list, 1);
	lv_obj_set_style_border_width(list, 0, 0);
	return list;
}

inline void add_back_button_event_cb(lv_obj_t* btn, std::function<void()>* onBack) {
	lv_obj_add_event_cb(
		btn,
		[](lv_event_t* e) {
			auto* callback = (std::function<void()>*)lv_event_get_user_data(e);
			if (callback && *callback) {
				(*callback)();
			}
		},
		LV_EVENT_CLICKED, onBack
	);
}

inline void add_switch_item(lv_obj_t* list, const char* text, lv_subject_t* subject) {
	lv_obj_t* btn = lv_list_add_button(list, nullptr, text);
	lv_obj_t* sw = lv_switch_create(btn);
	lv_obj_add_flag(sw, LV_OBJ_FLAG_EVENT_BUBBLE);
	lv_obj_align(sw, LV_ALIGN_RIGHT_MID, 0, 0);
	lv_obj_bind_checked(sw, subject);
}

inline lv_obj_t* add_slider_item(lv_obj_t* list, const char* text, lv_subject_t* subject, int32_t min_val, int32_t max_val) {
	lv_obj_t* btn = lv_list_add_button(list, nullptr, text);
	lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_t* slider = lv_slider_create(btn);
	lv_slider_set_range(slider, min_val, max_val);
	lv_obj_set_flex_grow(slider, 1);
	lv_slider_bind_value(slider, subject);

	lv_obj_t* value_label = lv_label_create(btn);
	lv_label_set_text_fmt(value_label, "%ld", lv_subject_get_int(subject));
	lv_obj_set_style_min_width(value_label, lv_dpx(UiConstants::PAD_LARGE), 0);

	// Update label when slider changes
	lv_obj_add_event_cb(slider, [](lv_event_t* e) {
		auto* label = (lv_obj_t*)lv_event_get_user_data(e);
		auto* slider_obj = lv_event_get_target_obj(e);
		lv_label_set_text_fmt(label, "%ld", lv_slider_get_value(slider_obj)); }, LV_EVENT_VALUE_CHANGED, value_label);

	return slider;
}

} // namespace flx::ui::common
