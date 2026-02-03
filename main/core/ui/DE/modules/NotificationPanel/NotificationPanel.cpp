#include "NotificationPanel.hpp"
#include "../../../theming/StyleUtils.hpp"
#include "core/system/Notification/NotificationManager.hpp"
#include "core/ui/theming/LayoutConstants/LayoutConstants.hpp"
#include "core/ui/theming/UiConstants/UiConstants.hpp"
#include <ctime>

namespace UI::Modules {

NotificationPanel::NotificationPanel(lv_obj_t* parent, lv_obj_t* statusBar)
	: m_parent(parent), m_statusBar(statusBar) {
	create();
}

void NotificationPanel::create() {
	m_panel = lv_obj_create(m_parent);
	// configure_panel_style(notification_panel) logic:
	lv_obj_set_size(m_panel, lv_pct(100), lv_pct(UiConstants::SIZE_NOTIF_PANEL_HEIGHT_PCT));
	lv_obj_set_style_pad_all(m_panel, 0, 0);
	lv_obj_set_style_radius(m_panel, lv_dpx(UiConstants::RADIUS_LARGE), 0);
	lv_obj_set_style_border_width(m_panel, 0, 0);
	lv_obj_add_flag(m_panel, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(m_panel, LV_OBJ_FLAG_FLOATING);
	UI::StyleUtils::apply_glass(m_panel, lv_dpx(UiConstants::GLASS_BLUR_SMALL));

	lv_obj_align_to(m_panel, m_statusBar, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
	lv_obj_set_flex_flow(m_panel, LV_FLEX_FLOW_COLUMN);

	lv_obj_t* header = lv_obj_create(m_panel);
	lv_obj_remove_style_all(header);
	lv_obj_set_size(header, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_all(header, lv_dpx(UiConstants::PAD_LARGE), 0);

	lv_obj_t* title = lv_label_create(header);
	lv_label_set_text(title, "Notifications");

	m_clearAllBtn = lv_button_create(header);
	lv_obj_set_size(m_clearAllBtn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_style_pad_all(m_clearAllBtn, lv_dpx(UiConstants::PAD_MEDIUM), 0);
	lv_obj_t* clear_label = lv_label_create(m_clearAllBtn);
	lv_label_set_text(clear_label, "Clear All");
	lv_obj_add_event_cb(m_clearAllBtn, on_clear_click, LV_EVENT_CLICKED, nullptr);

	m_list = lv_obj_create(m_panel);
	lv_obj_remove_style_all(m_list);
	lv_obj_set_width(m_list, lv_pct(100));
	lv_obj_set_flex_grow(m_list, 1);
	lv_obj_set_flex_flow(m_list, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_all(m_list, lv_dpx(UiConstants::PAD_MEDIUM), 0);
	lv_obj_set_style_pad_row(m_list, lv_dpx(UiConstants::PAD_MEDIUM), 0);

	lv_subject_add_observer_obj(
		&System::NotificationManager::getInstance().getUpdateSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			auto* instance = (NotificationPanel*)lv_observer_get_user_data(observer);
			if (instance) instance->update_list();
		},
		m_list, this
	);

	// Add swipe-up indicator
	lv_obj_t* up_indicator = lv_label_create(m_panel);
	lv_label_set_text(up_indicator, LV_SYMBOL_UP);
	lv_obj_set_width(up_indicator, lv_pct(100));
	lv_obj_set_style_text_align(up_indicator, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_opa(up_indicator, UiConstants::OPA_TEXT_DIM, 0);
	lv_obj_set_style_pad_bottom(up_indicator, lv_dpx(UiConstants::PAD_SMALL), 0);

	update_list();
}

void NotificationPanel::update_list() {
	if (!m_list) return;
	lv_obj_clean(m_list);

	const auto& notifs = System::NotificationManager::getInstance().getNotifications();

	if (notifs.empty()) {
		lv_obj_set_flex_align(m_list, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		lv_obj_t* empty_label = lv_label_create(m_list);
		lv_label_set_text(empty_label, "No new notifications");
		lv_obj_set_style_text_opa(empty_label, UiConstants::OPA_TEXT_DIM, 0);
		if (m_clearAllBtn) lv_obj_add_flag(m_clearAllBtn, LV_OBJ_FLAG_HIDDEN);
		return;
	}

	if (m_clearAllBtn) lv_obj_remove_flag(m_clearAllBtn, LV_OBJ_FLAG_HIDDEN);
	lv_obj_set_flex_align(m_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	for (const auto& n: notifs) {
		lv_obj_t* item = lv_obj_create(m_list);
		lv_obj_set_size(item, lv_pct(100), LV_SIZE_CONTENT);
		lv_obj_set_style_radius(item, lv_dpx(UiConstants::RADIUS_DEFAULT), 0);
		lv_obj_set_style_bg_opa(item, UiConstants::OPA_ITEM_BG, 0);
		lv_obj_set_flex_flow(item, LV_FLEX_FLOW_ROW);
		lv_obj_set_flex_align(item, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
		lv_obj_set_style_pad_all(item, lv_dpx(UiConstants::PAD_DEFAULT), 0);

		if (n.icon) {
			lv_obj_t* icon = lv_image_create(item);
			lv_image_set_src(icon, n.icon);
		}

		lv_obj_t* content = lv_obj_create(item);
		lv_obj_remove_style_all(content);
		lv_obj_set_size(content, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		lv_obj_set_flex_grow(content, 1);
		lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
		lv_obj_set_style_pad_hor(content, lv_dpx(UiConstants::PAD_SMALL), 0);

		lv_obj_t* header_cont = lv_obj_create(content);
		lv_obj_remove_style_all(header_cont);
		lv_obj_set_size(header_cont, lv_pct(100), LV_SIZE_CONTENT);
		lv_obj_set_flex_flow(header_cont, LV_FLEX_FLOW_ROW);
		lv_obj_set_flex_align(header_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		lv_obj_set_style_pad_column(header_cont, lv_dpx(UiConstants::PAD_SMALL), 0);

		lv_obj_t* title_lbl = lv_label_create(header_cont);
		lv_label_set_text(title_lbl, n.title.c_str());

		lv_obj_t* time_lbl = lv_label_create(header_cont);
		time_t ts = n.timestamp;
		struct tm timeinfo;
		localtime_r(&ts, &timeinfo);
		lv_label_set_text_fmt(time_lbl, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
		lv_obj_set_style_text_opa(time_lbl, UiConstants::OPA_TEXT_DIM, 0);

		lv_obj_t* msg_lbl = lv_label_create(content);
		lv_label_set_text(msg_lbl, n.message.c_str());
		lv_label_set_long_mode(msg_lbl, LV_LABEL_LONG_WRAP);
		lv_obj_set_width(msg_lbl, lv_pct(100));
		lv_obj_set_style_text_opa(msg_lbl, UiConstants::OPA_70, 0);

		lv_obj_t* close_btn = lv_button_create(item);
		lv_obj_set_size(close_btn, LayoutConstants::SIZE_TOUCH_TARGET, LayoutConstants::SIZE_TOUCH_TARGET);
		lv_obj_set_style_radius(close_btn, LV_RADIUS_CIRCLE, 0);
		lv_obj_t* close_icon = lv_label_create(close_btn);
		lv_label_set_text(close_icon, LV_SYMBOL_CLOSE);
		lv_obj_center(close_icon);
		lv_obj_set_style_text_opa(close_icon, UiConstants::OPA_TEXT_DIM, 0);

		std::string* id_ptr = new std::string(n.id);
		lv_obj_set_user_data(close_btn, id_ptr);
		lv_obj_add_event_cb(close_btn, on_close_notif_click, LV_EVENT_ALL, nullptr);
	}
}

void NotificationPanel::on_clear_click(lv_event_t* e) {
	System::NotificationManager::getInstance().clearAll();
}

void NotificationPanel::on_close_notif_click(lv_event_t* e) {
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* btn = lv_event_get_target_obj(e);
	std::string* id = (std::string*)lv_obj_get_user_data(btn);

	if (code == LV_EVENT_CLICKED) {
		if (id) System::NotificationManager::getInstance().removeNotification(*id);
	} else if (code == LV_EVENT_DELETE) {
		if (id) {
			delete id;
			lv_obj_set_user_data(btn, nullptr);
		}
	}
}

} // namespace UI::Modules
