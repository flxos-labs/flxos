#include "SwipeManager.hpp"

#include "../../../theming/ui_constants/UiConstants.hpp"
#include "core/lv_obj.h"
#include "core/lv_obj_event.h"
#include "core/lv_obj_pos.h"
#include "core/lv_obj_style.h"
#include "core/system/focus/FocusManager.hpp"
#include "display/lv_display.h"
#include "indev/lv_indev.h"
#include "lv_api_map_v8.h"
#include "misc/lv_area.h"
#include "misc/lv_event.h"
#include "misc/lv_types.h"
#include <algorithm>
#include <cstdint>

namespace UI::Modules {

static const char* TAG = "SwipeManager";

SwipeManager::SwipeManager(const Config& config) : m_config(config) {
	create_trigger_zone();
}

void SwipeManager::create_trigger_zone() {
	m_triggerZone = lv_obj_create(m_config.screen);
	lv_obj_remove_style_all(m_triggerZone);
	lv_obj_set_size(m_triggerZone, lv_pct(100), lv_dpx(UiConstants::SIZE_SWIPE_ZONE));
	lv_obj_add_flag(m_triggerZone, LV_OBJ_FLAG_FLOATING);
	lv_obj_add_flag(m_triggerZone, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_align(m_triggerZone, LV_ALIGN_TOP_MID, 0, 0);
	lv_obj_move_foreground(m_triggerZone);

	lv_obj_add_event_cb(m_triggerZone, on_swipe_zone_press, LV_EVENT_PRESSED, this);
	lv_obj_add_event_cb(m_triggerZone, on_swipe_zone_pressing, LV_EVENT_PRESSING, this);
	lv_obj_add_event_cb(m_triggerZone, on_swipe_zone_release, LV_EVENT_RELEASED, this);

	if (m_config.notificationPanel) {
		lv_obj_add_event_cb(m_config.notificationPanel, on_notif_panel_press, LV_EVENT_PRESSED, this);
		lv_obj_add_event_cb(m_config.notificationPanel, on_notif_panel_pressing, LV_EVENT_PRESSING, this);
		lv_obj_add_event_cb(m_config.notificationPanel, on_notif_panel_release, LV_EVENT_RELEASED, this);
	}
	if (m_config.notificationList) {
		lv_obj_add_event_cb(m_config.notificationList, on_notif_panel_press, LV_EVENT_PRESSED, this);
		lv_obj_add_event_cb(m_config.notificationList, on_notif_panel_pressing, LV_EVENT_PRESSING, this);
		lv_obj_add_event_cb(m_config.notificationList, on_notif_panel_release, LV_EVENT_RELEASED, this);
	}
}

void SwipeManager::on_swipe_zone_press(lv_event_t* e) {
	auto* self = (SwipeManager*)lv_event_get_user_data(e);
	if (!self || !self->m_config.notificationPanel) return;

	lv_indev_t* indev = lv_indev_active();
	if (indev) {
		lv_point_t point;
		lv_indev_get_point(indev, &point);
		self->m_swipeStartY = point.y;
		self->m_swipeActive = true;

		int32_t const h = lv_display_get_vertical_resolution(nullptr);
		int32_t const statusBarHeight = lv_obj_get_height(self->m_config.statusBar);
		lv_obj_remove_flag(self->m_config.notificationPanel, LV_OBJ_FLAG_HIDDEN);
		lv_obj_move_foreground(self->m_config.notificationPanel);
		lv_obj_set_y(self->m_config.notificationPanel, -h + statusBarHeight);
	}
}

void SwipeManager::on_swipe_zone_pressing(lv_event_t* e) {
	auto* self = (SwipeManager*)lv_event_get_user_data(e);
	if (!self || !self->m_swipeActive || !self->m_config.notificationPanel) return;

	lv_indev_t* indev = lv_indev_active();
	if (indev) {
		lv_point_t point;
		lv_indev_get_point(indev, &point);
		int32_t const h = lv_display_get_vertical_resolution(nullptr);
		int32_t const statusBarHeight = lv_obj_get_height(self->m_config.statusBar);
		int32_t const diff = point.y - self->m_swipeStartY;

		int32_t new_y = -h + statusBarHeight + diff;
		new_y = std::min(new_y, statusBarHeight);

		lv_obj_set_y(self->m_config.notificationPanel, new_y);
	}
}

void SwipeManager::on_swipe_zone_release(lv_event_t* e) {
	auto* self = (SwipeManager*)lv_event_get_user_data(e);
	if (!self || !self->m_swipeActive || !self->m_config.notificationPanel) return;

	int32_t const current_y = lv_obj_get_y(self->m_config.notificationPanel);
	int32_t const h = lv_display_get_vertical_resolution(nullptr);
	int32_t const statusBarHeight = lv_obj_get_height(self->m_config.statusBar);

	if (current_y > -h + statusBarHeight + (h / 6)) {
		lv_obj_set_y(self->m_config.notificationPanel, statusBarHeight);
		System::FocusManager::getInstance().activatePanel(self->m_config.notificationPanel);
	} else {
		lv_obj_set_y(self->m_config.notificationPanel, -h + statusBarHeight);
		lv_obj_add_flag(self->m_config.notificationPanel, LV_OBJ_FLAG_HIDDEN);
	}
	self->m_swipeActive = false;
}

void SwipeManager::on_notif_panel_press(lv_event_t* e) {
	auto* self = (SwipeManager*)lv_event_get_user_data(e);
	if (!self) return;

	lv_indev_t* indev = lv_indev_active();
	if (indev) {
		lv_point_t point;
		lv_indev_get_point(indev, &point);
		self->m_swipeStartY = point.y;
		self->m_swipeActive = true;
	}
}

void SwipeManager::on_notif_panel_pressing(lv_event_t* e) {
	auto* self = (SwipeManager*)lv_event_get_user_data(e);
	if (!self || !self->m_swipeActive || !self->m_config.notificationPanel) return;

	lv_indev_t* indev = lv_indev_active();
	if (indev) {
		lv_point_t point;
		lv_indev_get_point(indev, &point);
		int32_t const h = lv_display_get_vertical_resolution(nullptr);
		int32_t const diff = point.y - self->m_swipeStartY;

		int32_t const statusBarHeight = lv_obj_get_height(self->m_config.statusBar);
		int32_t new_y = statusBarHeight + diff;
		new_y = std::min(new_y, statusBarHeight);
		new_y = std::max(new_y, -h + statusBarHeight);

		lv_obj_set_y(self->m_config.notificationPanel, new_y);
	}
}

void SwipeManager::on_notif_panel_release(lv_event_t* e) {
	auto* self = (SwipeManager*)lv_event_get_user_data(e);
	if (!self || !self->m_swipeActive || !self->m_config.notificationPanel) return;

	int32_t const current_y = lv_obj_get_y(self->m_config.notificationPanel);
	int32_t const h = lv_display_get_vertical_resolution(nullptr);
	int32_t const statusBarHeight = lv_obj_get_height(self->m_config.statusBar);

	if (current_y < statusBarHeight - (h / 6)) {
		lv_obj_set_y(self->m_config.notificationPanel, -h + statusBarHeight);
		System::FocusManager::getInstance().dismissAllPanels();
	} else {
		lv_obj_set_y(self->m_config.notificationPanel, statusBarHeight);
	}
	self->m_swipeActive = false;
}

} // namespace UI::Modules
