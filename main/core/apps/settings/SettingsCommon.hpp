#pragma once

#include "../../ui/theming/LayoutConstants.hpp"
#include "lvgl.h"
#include <functional>

namespace System {
namespace Apps {
namespace Settings {

inline lv_obj_t* add_list_btn(lv_obj_t* list, const char* symbol, const char* text) {
	lv_obj_t* btn = lv_list_add_button(list, NULL, text);
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
	return cont;
}

inline lv_obj_t* create_header(lv_obj_t* parent, const char* title_text, lv_obj_t** back_btn_out = nullptr) {
	lv_obj_t* header = lv_obj_create(parent);
	lv_obj_set_size(header, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_style_max_height(header, lv_dpx(UILayout::SIZE_TAB_BAR), 0);
	lv_obj_set_style_pad_all(header, 0, 0);
	lv_obj_set_style_pad_gap(header, lv_dpx(UILayout::PAD_SMALL), 0);
	lv_obj_set_style_border_width(header, 0, 0);
	lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, 0);
	lv_obj_set_style_border_width(header, lv_dpx(UILayout::BORDER_THIN), 0);
	lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(header, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_t* backBtn = lv_button_create(header);
	lv_obj_t* backLabel = lv_image_create(backBtn);
	lv_image_set_src(backLabel, LV_SYMBOL_LEFT);
	if (back_btn_out)
		*back_btn_out = backBtn;

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
			if (callback && *callback)
				(*callback)();
		},
		LV_EVENT_CLICKED, onBack
	);
}

} // namespace Settings
} // namespace Apps
} // namespace System
