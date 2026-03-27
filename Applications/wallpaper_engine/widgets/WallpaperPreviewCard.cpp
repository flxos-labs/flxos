#include "WallpaperPreviewCard.hpp"

#include <flx/ui/common/SettingsCommon.hpp>
#include <flx/ui/theming/ui_constants/UiConstants.hpp>

using namespace flx::ui::common;

namespace System::Apps::WallpaperEngine {

WallpaperPreviewCard::WallpaperPreviewCard(
	lv_obj_t* parent,
	const flx::ui::wallpaper::WallpaperPreset& preset,
	std::function<void(const std::string& id)> onApply) {

	// Container row
	lv_obj_t* card = lv_obj_create(parent);
	lv_obj_set_size(card, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_style_pad_hor(card, lv_dpx(UiConstants::PAD_SMALL), 0);
	lv_obj_set_style_pad_ver(card, lv_dpx(UiConstants::PAD_TINY), 0);
	lv_obj_set_style_border_width(card, 0, 0);
	lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	// Icon based on type
	const char* icon = LV_SYMBOL_IMAGE;
	if (preset.type == "dynamic") {
		icon = LV_SYMBOL_LOOP;
	} else if (preset.type == "animated" || preset.type == "lottie") {
		icon = LV_SYMBOL_PLAY;
	}

	lv_obj_t* iconImg = lv_image_create(card);
	lv_image_set_src(iconImg, icon);
	lv_obj_set_style_margin_right(iconImg, lv_dpx(UiConstants::PAD_SMALL), 0);

	// Text column
	lv_obj_t* textCol = lv_obj_create(card);
	lv_obj_set_style_pad_all(textCol, 0, 0);
	lv_obj_set_style_border_width(textCol, 0, 0);
	lv_obj_set_flex_grow(textCol, 1);
	lv_obj_set_flex_flow(textCol, LV_FLEX_FLOW_COLUMN);

	lv_obj_t* nameLabel = lv_label_create(textCol);
	lv_label_set_text(nameLabel, preset.name.c_str());

	if (!preset.description.empty()) {
		lv_obj_t* descLabel = lv_label_create(textCol);
		lv_label_set_text(descLabel, preset.description.c_str());
		lv_obj_set_style_text_opa(descLabel, LV_OPA_60, 0);
	}

	// Apply button — callback data is heap-allocated and freed on LV_EVENT_DELETE
	// so no dangling pointer occurs if the button is deleted before this C++ object.
	auto* cbData = new CallbackData{preset.id, std::move(onApply)};

	lv_obj_t* applyBtn = lv_button_create(card);
	lv_obj_t* applyLabel = lv_label_create(applyBtn);
	lv_label_set_text(applyLabel, "Apply");
	lv_obj_set_size(applyBtn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

	lv_obj_add_event_cb(
		applyBtn,
		[](lv_event_t* e) {
			auto* data = static_cast<CallbackData*>(lv_event_get_user_data(e));
			if (data && data->on_apply) {
				data->on_apply(data->preset_id);
			}
		},
		LV_EVENT_CLICKED, cbData);

	lv_obj_add_event_cb(
		applyBtn,
		[](lv_event_t* e) {
			auto* data = static_cast<CallbackData*>(lv_event_get_user_data(e));
			delete data;
		},
		LV_EVENT_DELETE, cbData);
}

} // namespace System::Apps::WallpaperEngine
