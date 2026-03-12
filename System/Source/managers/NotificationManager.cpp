#include <flx/system/managers/NotificationManager.hpp>

#include "esp_timer.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <flx/core/Logger.hpp>
#include <sstream>
#include <string_view>

static constexpr std::string_view TAG = "Notification";

namespace flx::system {

const flx::services::ServiceManifest NotificationManager::serviceManifest = {
	.serviceId = "com.flxos.notifications",
	.serviceName = "Notifications",
	.dependencies = {},
	.priority = 80,
	.required = false,
	.autoStart = true,
	.guiRequired = true,
	.capabilities = flx::services::ServiceCapability::None,
	.description = "System notification management",
};

NotificationManager::NotificationManager() {
}

bool NotificationManager::onStart() {
	Log::info(TAG, "Notification service started");
	return true;
}

void NotificationManager::onStop() {
	clearAll();
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_latest_notification = {};
		m_latest_notification_serial = 0;
	}
	Log::info(TAG, "Notification service stopped");
}

std::string NotificationManager::generateId() {
	static int counter = 0;
	std::stringstream ss;
	ss << "notif_" << esp_timer_get_time() << "_" << counter++;
	return ss.str();
}

void NotificationManager::addNotification(const std::string& title, const std::string& message, const std::string& appName, const void* icon, int priority) {
	Notification notif;
	int32_t latestSerial = 0;
	{ // Add scope block
		std::lock_guard<std::mutex> lock(m_mutex);
		Log::info(TAG, "New notification from %s: %s", appName.c_str(), title.c_str());

		notif.id = generateId();
		notif.title = title;
		notif.message = message;
		notif.appName = appName;
		notif.icon = icon;
		notif.priority = priority;
		notif.timestamp = (uint32_t)(esp_timer_get_time() / 1000000);
		notif.isRead = false;

		m_notifications.insert(m_notifications.begin(), notif);
		m_latest_notification = notif;
		latestSerial = ++m_latest_notification_serial;
	} // lock_guard destructor releases mutex here

	updateSubjects();
	m_latest_notification_subject.setAndNotify(latestSerial);
}

void NotificationManager::removeNotification(const std::string& id) {
	bool changed = false;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		auto it = std::remove_if(m_notifications.begin(), m_notifications.end(), [&id](const Notification& n) { return n.id == id; });

		if (it != m_notifications.end()) {
			m_notifications.erase(it, m_notifications.end());
			changed = true;

			if (m_latest_notification.id == id) {
				m_latest_notification = {};
				m_latest_notification_serial = 0;
			}
		}
	}
	if (changed) {
		updateSubjects();
	}
}

void NotificationManager::clearAll() {
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		Log::info(TAG, "Clearing all notifications (%zu count)", m_notifications.size());
		m_notifications.clear();
		m_latest_notification = {};
		m_latest_notification_serial = 0;
	}
	updateSubjects();
}

void NotificationManager::markAsRead(const std::string& id) {
	bool changed = false;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		for (auto& n: m_notifications) {
			if (n.id == id && !n.isRead) {
				n.isRead = true;
				changed = true;
				break;
			}
		}
	}
	if (changed) {
		updateSubjects();
	}
}

void NotificationManager::markAllAsRead() {
	bool changed = false;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		for (auto& n: m_notifications) {
			if (!n.isRead) {
				n.isRead = true;
				changed = true;
			}
		}
	}
	if (changed) {
		updateSubjects();
	}
}

std::vector<Notification> NotificationManager::getNotifications() const {
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_notifications;
}

size_t NotificationManager::getUnreadCount() const {
	std::lock_guard<std::mutex> lock(m_mutex);
	size_t count = 0;
	for (const auto& n: m_notifications) {
		if (!n.isRead) count++;
	}
	return count;
}

bool NotificationManager::tryGetLatestNotification(Notification& out) const {
	std::lock_guard<std::mutex> lock(m_mutex);
	if (m_latest_notification_serial <= 0 || m_latest_notification.id.empty()) {
		return false;
	}

	out = m_latest_notification;
	return true;
}

void NotificationManager::updateSubjects() {
	m_unread_count_subject.set((int32_t)getUnreadCount());
	int32_t const current = m_update_subject.get();
	m_update_subject.setAndNotify(current + 1);
}

} // namespace flx::system
