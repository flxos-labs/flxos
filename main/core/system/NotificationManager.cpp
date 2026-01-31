#include "NotificationManager.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include <algorithm>
#include <random>
#include <sstream>

static const char* TAG = "NotificationManager";

namespace System {

NotificationManager& NotificationManager::getInstance() {
	static NotificationManager instance;
	return instance;
}

NotificationManager::NotificationManager() {
	lv_subject_init_int(&m_unread_count_subject, 0);
	lv_subject_init_int(&m_update_subject, 0);
}

void NotificationManager::init() {
	ESP_LOGI(TAG, "NotificationManager initialized");
}

std::string NotificationManager::generateId() {
	static int counter = 0;
	std::stringstream ss;
	ss << "notif_" << esp_timer_get_time() << "_" << counter++;
	return ss.str();
}

void NotificationManager::addNotification(const std::string& title, const std::string& message, const std::string& appName, const void* icon, int priority) {
	std::lock_guard<std::mutex> lock(m_mutex);

	Notification notif;
	notif.id = generateId();
	notif.title = title;
	notif.message = message;
	notif.appName = appName;
	notif.icon = icon;
	notif.priority = priority;
	notif.timestamp = (uint32_t)(esp_timer_get_time() / 1000000); // Seconds
	notif.isRead = false;

	// Add to beginning (newest first)
	m_notifications.insert(m_notifications.begin(), notif);

	ESP_LOGI(TAG, "Added notification: %s", title.c_str());
	updateSubjects();
}

void NotificationManager::removeNotification(const std::string& id) {
	std::lock_guard<std::mutex> lock(m_mutex);
	auto it = std::remove_if(m_notifications.begin(), m_notifications.end(), [&id](const Notification& n) { return n.id == id; });

	if (it != m_notifications.end()) {
		m_notifications.erase(it, m_notifications.end());
		ESP_LOGD(TAG, "Removed notification: %s", id.c_str());
		updateSubjects();
	}
}

void NotificationManager::clearAll() {
	std::lock_guard<std::mutex> lock(m_mutex);
	m_notifications.clear();
	ESP_LOGI(TAG, "Cleared all notifications");
	updateSubjects();
}

void NotificationManager::markAsRead(const std::string& id) {
	std::lock_guard<std::mutex> lock(m_mutex);
	for (auto& n: m_notifications) {
		if (n.id == id && !n.isRead) {
			n.isRead = true;
			updateSubjects();
			break;
		}
	}
}

void NotificationManager::markAllAsRead() {
	std::lock_guard<std::mutex> lock(m_mutex);
	bool changed = false;
	for (auto& n: m_notifications) {
		if (!n.isRead) {
			n.isRead = true;
			changed = true;
		}
	}
	if (changed) {
		updateSubjects();
	}
}

const std::vector<Notification>& NotificationManager::getNotifications() const {
	return m_notifications;
}

size_t NotificationManager::getUnreadCount() const {
	size_t count = 0;
	for (const auto& n: m_notifications) {
		if (!n.isRead) count++;
	}
	return count;
}

void NotificationManager::updateSubjects() {
	// These must be called in a way that is thread-safe with LVGL if called from another task
	// Since this is generic C++ logic, assuming caller context or subjects handle thread safety
	// LVGL 9 subjects are generally thread safe if lv_lock is used?
	// Usually these are read from UI task.
	// If called from other tasks, we might need a mutex or dispatch to UI task.
	// For now, assuming these simply update integer values.

	lv_subject_set_int(&m_unread_count_subject, (int32_t)getUnreadCount());

	// Toggle or increment update subject to signal list change
	int current = lv_subject_get_int(&m_update_subject);
	lv_subject_set_int(&m_update_subject, current + 1);
}

} // namespace System
