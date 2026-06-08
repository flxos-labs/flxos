#include <flx/core/EventBus.hpp>
#include <flx/system/managers/NotificationManager.hpp>
#include <font/lv_symbol_def.h>

#include "esp_heap_caps.h"
#include "esp_timer.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <flx/core/Logger.hpp>
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

	// Listen for remote notifications via EventBus (e.g. from Kernel/Core)
	m_event_sub_id = flx::core::EventBus::getInstance().subscribe("system.notify", [this](const std::string& /*event*/, const flx::core::Bundle& data) {
		if (!this->isRunning()) return;

		std::string title = data.getStringOr("title", "Alert");
		std::string message = data.getStringOr("message", "");
		std::string appName = data.getStringOr("appName", "System");
		int priority = data.getInt32Or("priority", 1);

		// Map icon string to LVGL symbol if needed
		const void* icon = nullptr;
		std::string iconStr = data.getStringOr("icon", "");
		if (iconStr == "warning") icon = LV_SYMBOL_WARNING;
		else if (iconStr == "info")
			icon = LV_SYMBOL_LIST;
		else if (iconStr == "error")
			icon = LV_SYMBOL_CLOSE;
		else if (iconStr == "save")
			icon = LV_SYMBOL_SAVE;
		else if (iconStr == "wifi")
			icon = LV_SYMBOL_WIFI;
		else if (iconStr == "battery")
			icon = LV_SYMBOL_BATTERY_EMPTY;
		else if (iconStr == "refresh")
			icon = LV_SYMBOL_REFRESH;

		this->addNotification(title, message, appName, icon, priority);
	});

	return true;
}

void NotificationManager::onStop() {
	if (m_event_sub_id != 0) {
		flx::core::EventBus::getInstance().unsubscribe(m_event_sub_id);
		m_event_sub_id = 0;
	}
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
	char id[48] = {0};
	std::snprintf(id,
		sizeof(id),
		"notif_%llu_%d",
		static_cast<unsigned long long>(esp_timer_get_time()),
		counter++);
	return std::string(id);
}

void NotificationManager::addNotification(const std::string& title, const std::string& message, const std::string& appName, const void* icon, int priority) {
	if (!isRunning()) {
		Log::warn(TAG, "Dropping notification while service is stopped: %s", title.c_str());
		return;
	}

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
		notif.timestamp = (uint32_t)time(nullptr);
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
