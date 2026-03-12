#pragma once

#include "lvgl.h"
#include <deque>
#include <flx/system/managers/NotificationManager.hpp>
#include <memory>
#include <string>

namespace UI::Modules {

class FloatingNotifications {
public:

	FloatingNotifications(lv_obj_t* parent, lv_obj_t* statusBar);
	~FloatingNotifications();

	void realign();

private:

	void create();
	void enqueueNotification(const flx::system::Notification& notification);
	void showNotification(const flx::system::Notification& notification);
	void dismissCurrentNotification();
	uint32_t getDisplayDurationMs(int priority) const;
	static void on_timer(lv_timer_t* timer);
	static void on_container_event(lv_event_t* e);

	lv_obj_t* m_parent;
	lv_obj_t* m_statusBar;
	lv_obj_t* m_container = nullptr;
	lv_obj_t* m_card = nullptr;
	lv_obj_t* m_icon = nullptr;
	lv_obj_t* m_titleLabel = nullptr;
	lv_obj_t* m_timeLabel = nullptr;
	lv_obj_t* m_messageLabel = nullptr;
	lv_timer_t* m_timer = nullptr;
	size_t m_observerId = 0;
	std::deque<flx::system::Notification> m_pendingNotifications {};
	std::string m_activeNotificationId {};
	std::shared_ptr<bool> m_alive = std::make_shared<bool>(true);
};

} // namespace UI::Modules
