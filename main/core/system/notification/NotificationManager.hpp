#pragma once

#include "lvgl.h"
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace System {

struct Notification {
	std::string id {};
	std::string title {};
	std::string message {};
	std::string appName {};
	const void* icon {}; // LVGL image source
	int priority {}; // 0: Low, 1: Normal, 2: High
	uint32_t timestamp {};
	bool isRead {};
};

class NotificationManager {
public:

	static NotificationManager& getInstance();

	void init();

	// Notification Management
	void addNotification(const std::string& title, const std::string& message, const std::string& appName = "System", const void* icon = nullptr, int priority = 1);
	void removeNotification(const std::string& id);
	void clearAll();
	void markAsRead(const std::string& id);
	void markAllAsRead();

	// Getters
	const std::vector<Notification>& getNotifications() const;
	size_t getUnreadCount() const;

	// Subject for UI binding
	// Value will be the number of unread notifications
	lv_subject_t& getUnreadCountSubject() { return m_unread_count_subject; }

	// Value will be incremented on any change (add/remove/read) to signal UI update
	lv_subject_t& getUpdateSubject() { return m_update_subject; }

private:

	NotificationManager();
	~NotificationManager() = default;
	NotificationManager(const NotificationManager&) = delete;
	NotificationManager& operator=(const NotificationManager&) = delete;

	std::vector<Notification> m_notifications {};
	mutable std::mutex m_mutex {};
	lv_subject_t m_unread_count_subject {};
	lv_subject_t m_update_subject {};

	std::string generateId();
	void updateSubjects();
};

} // namespace System
