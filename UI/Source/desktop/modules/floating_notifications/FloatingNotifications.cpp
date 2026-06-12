#include "core/lv_obj.h"
#include "core/lv_obj_event.h"
#include "core/lv_obj_pos.h"
#include "core/lv_obj_scroll.h"
#include "core/lv_obj_style.h"
#include "core/lv_obj_style_gen.h"
#include "misc/lv_event.h"
#include "misc/lv_timer.h"
#include "widgets/image/lv_image.h"
#include "widgets/label/lv_label.h"
#include <ctime>
#include <flx/apps/AppManager.hpp>
#include <flx/system/managers/NotificationManager.hpp>
#include <flx/ui/GuiTask.hpp>
#include <flx/ui/desktop/modules/floating_notifications/FloatingNotifications.hpp>
#include <flx/ui/theming/StyleUtils.hpp>
#include <flx/ui/theming/ui_constants/UiConstants.hpp>

namespace UI::Modules {

namespace {

constexpr uint32_t LOW_PRIORITY_DURATION_MS = 3000;
constexpr uint32_t NORMAL_PRIORITY_DURATION_MS = 4500;
constexpr uint32_t HIGH_PRIORITY_DURATION_MS = 6500;

} // namespace

FloatingNotifications::FloatingNotifications(lv_obj_t* parent, lv_obj_t* statusBar)
	: m_parent(parent), m_statusBar(statusBar) {
	create();
}

FloatingNotifications::~FloatingNotifications() {
	*m_alive = false;
	flx::system::NotificationManager::getInstance().getLatestNotificationObservable().unsubscribe(m_observerId);
	if (m_timer) {
		lv_timer_delete(m_timer);
		m_timer = nullptr;
	}
}

void FloatingNotifications::create() {
	m_container = lv_obj_create(m_parent);
	lv_obj_set_size(m_container, lv_pct(90), LV_SIZE_CONTENT);
	lv_obj_remove_flag(m_container, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(m_container, LV_OBJ_FLAG_FLOATING);
	lv_obj_add_flag(m_container, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(m_container, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_pad_all(m_container, lv_dpx(UiConstants::PAD_SMALL), 0);
	lv_obj_set_style_radius(m_container, lv_dpx(UiConstants::RADIUS_LARGE), 0);
	lv_obj_set_style_border_width(m_container, 0, 0);
	UI::StyleUtils::apply_glass(m_container, lv_dpx(UiConstants::GLASS_BLUR_SMALL));
	lv_obj_add_event_cb(m_container, on_container_event, LV_EVENT_CLICKED, this);

	m_card = lv_obj_create(m_container);
	lv_obj_set_width(m_card, lv_pct(100));
	lv_obj_set_height(m_card, LV_SIZE_CONTENT);
	lv_obj_remove_flag(m_card, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_radius(m_card, lv_dpx(UiConstants::RADIUS_DEFAULT), 0);
	lv_obj_set_style_bg_opa(m_card, UiConstants::OPA_ITEM_BG, 0);
	lv_obj_set_style_border_width(m_card, 0, 0);
	lv_obj_set_flex_flow(m_card, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(m_card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_all(m_card, lv_dpx(UiConstants::PAD_DEFAULT), 0);

	m_icon = lv_image_create(m_card);
	lv_obj_add_flag(m_icon, LV_OBJ_FLAG_HIDDEN);

	lv_obj_t* content = lv_obj_create(m_card);
	lv_obj_remove_style_all(content);
	lv_obj_set_size(content, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_flex_grow(content, 1);
	lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_hor(content, lv_dpx(UiConstants::PAD_SMALL), 0);
	lv_obj_set_style_pad_row(content, lv_dpx(UiConstants::PAD_TINY), 0);

	lv_obj_t* header = lv_obj_create(content);
	lv_obj_remove_style_all(header);
	lv_obj_set_size(header, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_column(header, lv_dpx(UiConstants::PAD_SMALL), 0);

	m_titleLabel = lv_label_create(header);
	lv_obj_set_flex_grow(m_titleLabel, 1);
	lv_label_set_long_mode(m_titleLabel, LV_LABEL_LONG_WRAP);

	m_timeLabel = lv_label_create(header);
	lv_obj_set_style_text_opa(m_timeLabel, UiConstants::OPA_TEXT_DIM, 0);

	m_messageLabel = lv_label_create(content);
	lv_obj_set_width(m_messageLabel, lv_pct(100));
	lv_label_set_long_mode(m_messageLabel, LV_LABEL_LONG_WRAP);
	lv_obj_set_style_text_opa(m_messageLabel, UiConstants::OPA_70, 0);

	m_timer = lv_timer_create(on_timer, NORMAL_PRIORITY_DURATION_MS, this);
	lv_timer_pause(m_timer);

	std::weak_ptr<bool> weak_alive = m_alive;
	m_observerId = flx::system::NotificationManager::getInstance().getLatestNotificationObservable().subscribe(
		[this, weak_alive](int32_t) {
			flx::system::Notification notification;
			if (!flx::system::NotificationManager::getInstance().tryGetLatestNotification(notification)) {
				return;
			}

			flx::ui::GuiTask::perform([this, weak_alive, notification] {
				if (auto alive = weak_alive.lock(); alive && *alive) {
					this->enqueueNotification(notification);
				}
			});
		});

	flx::system::Notification latestNotification;
	if (flx::system::NotificationManager::getInstance().tryGetLatestNotification(latestNotification)) {
		enqueueNotification(latestNotification);
	}

	realign();
}

void FloatingNotifications::realign() {
	if (!m_container || !m_statusBar) {
		return;
	}

	lv_obj_align_to(
		m_container,
		m_statusBar,
		LV_ALIGN_OUT_BOTTOM_MID,
		0,
		lv_dpx(UiConstants::PAD_SMALL));
}

void FloatingNotifications::enqueueNotification(const flx::system::Notification& notification) {
	if (notification.id.empty()) {
		return;
	}

	if (!m_activeNotificationId.empty()) {
		m_pendingNotifications.push_back(notification);
		return;
	}

	showNotification(notification);
}

void FloatingNotifications::showNotification(const flx::system::Notification& notification) {
	if (!m_container || !m_card || !m_icon || !m_titleLabel || !m_timeLabel || !m_messageLabel) {
		return;
	}

	std::string const title = notification.title.empty() ? "Notification" : notification.title;

	m_activeNotificationId = notification.id;
	lv_label_set_text(m_titleLabel, title.c_str());
	lv_label_set_text(m_messageLabel, notification.message.c_str());

	if (notification.icon) {
		lv_image_set_src(m_icon, notification.icon);
		lv_obj_remove_flag(m_icon, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_obj_add_flag(m_icon, LV_OBJ_FLAG_HIDDEN);
	}

	time_t ts = notification.timestamp;
	struct tm timeinfo;
	localtime_r(&ts, &timeinfo);
	lv_label_set_text_fmt(m_timeLabel, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);

	if (notification.message.empty()) {
		lv_obj_add_flag(m_messageLabel, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_obj_remove_flag(m_messageLabel, LV_OBJ_FLAG_HIDDEN);
	}

	realign();
	lv_obj_move_foreground(m_container);
	lv_obj_remove_flag(m_container, LV_OBJ_FLAG_HIDDEN);
	lv_timer_set_period(m_timer, getDisplayDurationMs(notification.priority));
	lv_timer_reset(m_timer);
	lv_timer_resume(m_timer);
}

void FloatingNotifications::dismissCurrentNotification() {
	if (m_timer) {
		lv_timer_pause(m_timer);
	}

	m_activeNotificationId.clear();
	if (m_container) {
		lv_obj_add_flag(m_container, LV_OBJ_FLAG_HIDDEN);
	}

	if (!m_pendingNotifications.empty()) {
		flx::system::Notification next = m_pendingNotifications.front();
		m_pendingNotifications.pop_front();
		showNotification(next);
	}
}

uint32_t FloatingNotifications::getDisplayDurationMs(int priority) const {
	if (priority >= 2) {
		return HIGH_PRIORITY_DURATION_MS;
	}
	if (priority <= 0) {
		return LOW_PRIORITY_DURATION_MS;
	}
	return NORMAL_PRIORITY_DURATION_MS;
}

void FloatingNotifications::on_timer(lv_timer_t* timer) {
	auto* self = static_cast<FloatingNotifications*>(lv_timer_get_user_data(timer));
	if (self) {
		self->dismissCurrentNotification();
	}
}

void FloatingNotifications::on_container_event(lv_event_t* e) {
	if (lv_event_get_code(e) != LV_EVENT_CLICKED) {
		return;
	}

	auto* self = static_cast<FloatingNotifications*>(lv_event_get_user_data(e));
	if (self && !self->m_activeNotificationId.empty()) {
		// Find the notification to get redirect details
		const auto& notifs = flx::system::NotificationManager::getInstance().getNotifications();
		for (const auto& n: notifs) {
			if (n.id == self->m_activeNotificationId) {
				if (!n.redirectAppId.empty()) {
					flx::apps::Intent intent;
					intent.targetAppId = n.redirectAppId;
					intent.data = n.redirectData;
					intent.action = n.redirectData.empty() ? flx::apps::IntentAction::ACTION_MAIN : flx::apps::IntentAction::ACTION_VIEW;
					flx::apps::AppManager::getInstance().startApp(intent);
				}
				// Remove the notification from manager
				flx::system::NotificationManager::getInstance().removeNotification(n.id);
				break;
			}
		}
		self->dismissCurrentNotification();
	}
}

} // namespace UI::Modules
