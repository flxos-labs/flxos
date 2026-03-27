#include "EffectSliderRow.hpp"

#include <flx/ui/theming/ui_constants/UiConstants.hpp>

namespace System::Apps::WallpaperEngine {

EffectSliderRow::EffectSliderRow(
	lv_obj_t* parent,
	const char* label,
	lv_subject_t* subject,
	int32_t min_val,
	int32_t max_val) {

	// Row container
	lv_obj_t* row = lv_obj_create(parent);
	lv_obj_set_size(row, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_style_pad_hor(row, lv_dpx(UiConstants::PAD_SMALL), 0);
	lv_obj_set_style_pad_ver(row, lv_dpx(UiConstants::PAD_TINY), 0);
	lv_obj_set_style_border_width(row, 0, 0);
	lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	// Label
	lv_obj_t* nameLabel = lv_label_create(row);
	lv_label_set_text(nameLabel, label);
	lv_obj_set_style_min_width(nameLabel, lv_dpx(60), 0);

	// Slider
	lv_obj_t* slider = lv_slider_create(row);
	lv_slider_set_range(slider, min_val, max_val);
	lv_obj_set_flex_grow(slider, 1);
	if (subject != nullptr) {
		lv_slider_bind_value(slider, subject);
	}

	// Value label
	lv_obj_t* valueLabel = lv_label_create(row);
	int32_t const initial = (subject != nullptr) ? lv_subject_get_int(subject) : min_val;
	lv_label_set_text_fmt(valueLabel, "%ld", initial);
	lv_obj_set_style_min_width(valueLabel, lv_dpx(UiConstants::PAD_LARGE), 0);

	// Keep value label in sync
	lv_obj_add_event_cb(
		slider,
		[](lv_event_t* e) {
			auto* valLabel = static_cast<lv_obj_t*>(lv_event_get_user_data(e));
			auto* s = lv_event_get_target_obj(e);
			lv_label_set_text_fmt(valLabel, "%ld", lv_slider_get_value(s));
		},
		LV_EVENT_VALUE_CHANGED, valueLabel);
}

} // namespace System::Apps::WallpaperEngine
