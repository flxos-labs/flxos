#include "Dock.hpp"
#include "core/ui/theming/LayoutConstants/LayoutConstants.hpp"
#include "core/ui/theming/StyleUtils.hpp"
#include "core/ui/theming/UiConstants/UiConstants.hpp"

namespace UI::Modules {

Dock::Dock(lv_obj_t* parent, Callbacks callbacks)
	: m_parent(parent), m_callbacks(callbacks) {
	create();
}

lv_obj_t* Dock::create_dock_btn(lv_obj_t* parent, const char* icon, int32_t w, int32_t h) {
	lv_obj_t* btn = lv_button_create(parent);
	lv_obj_set_size(btn, w, h);
	lv_obj_set_style_radius(btn, lv_dpx(UiConstants::RADIUS_SMALL), 0);
	lv_obj_t* img = lv_image_create(btn);
	lv_image_set_src(img, icon);
	lv_obj_center(img);
	return btn;
}

void Dock::create() {
	m_dock = lv_obj_create(m_parent);
	lv_obj_remove_style_all(m_dock);
	lv_obj_set_size(m_dock, lv_pct(UiConstants::SIZE_DOCK_WIDTH_PCT), lv_pct(UiConstants::SIZE_DOCK_HEIGHT_PCT));
	lv_obj_set_style_pad_hor(m_dock, lv_dpx(UiConstants::PAD_SMALL), 0);
	lv_obj_set_style_radius(m_dock, lv_dpx(UiConstants::RADIUS_DEFAULT), 0);
	lv_obj_set_style_margin_bottom(m_dock, lv_dpx(UiConstants::PAD_SMALL), 0);

	UI::StyleUtils::apply_glass(m_dock, lv_dpx(UiConstants::GLASS_BLUR_LARGE));

	lv_obj_set_flex_flow(m_dock, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(m_dock, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_t* start_btn = create_dock_btn(m_dock, LV_SYMBOL_LIST, lv_pct(LayoutConstants::DOCK_BTN_WIDTH_PCT), lv_pct(LayoutConstants::DOCK_BTN_HEIGHT_PCT));
	lv_obj_add_event_cb(start_btn, [](lv_event_t* e) {
        auto* cb = (std::function<void()>*)lv_event_get_user_data(e);
        if (cb && *cb) (*cb)(); }, LV_EVENT_CLICKED, &m_callbacks.onStartClick);

	m_appContainer = lv_obj_create(m_dock);
	lv_obj_remove_style_all(m_appContainer);
	lv_obj_set_size(m_appContainer, 0, lv_pct(100));
	lv_obj_set_style_pad_hor(m_appContainer, lv_dpx(UiConstants::PAD_SMALL), 0);
	lv_obj_set_flex_grow(m_appContainer, 1);
	lv_obj_set_flex_flow(m_appContainer, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(m_appContainer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_t* up_btn = create_dock_btn(m_dock, LV_SYMBOL_UP, lv_pct(LayoutConstants::DOCK_BTN_WIDTH_PCT), lv_pct(LayoutConstants::DOCK_BTN_HEIGHT_SMALL_PCT));
	lv_obj_add_event_cb(up_btn, [](lv_event_t* e) {
        auto* cb = (std::function<void()>*)lv_event_get_user_data(e);
        if (cb && *cb) (*cb)(); }, LV_EVENT_CLICKED, &m_callbacks.onUpClick);
}

} // namespace UI::Modules
