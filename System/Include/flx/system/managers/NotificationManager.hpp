#pragma once

#include <flx/core/EventBus.hpp>
#include <flx/core/Observable.hpp>
#include <flx/core/Singleton.hpp>
#include <flx/services/IService.hpp>
#include <flx/services/ServiceManifest.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace flx::system {

struct Notification {
	std::string id {};
	std::string title {};
	std::string message {};
	std::string appName {};
	const void* icon {}; // LVGL image source (opaque pointer)
	int priority {}; // 0: Low, 1: Normal, 2: High
	uint32_t timestamp {};
	bool isRead {};
	bool dismissable {true};
	std::string redirectAppId {};
	std::string redirectData {};
};

class NotificationManager : public flx::Singleton<NotificationManager>, public flx::services::IService {
	friend class flx::Singleton<NotificationManager>;

public:

	// ──── IService manifest ────
	static const flx::services::ServiceManifest serviceManifest;
	const flx::services::ServiceManifest& getManifest() const override { return serviceManifest; }

	// ──── IService lifecycle ────
	bool onStart() override;
	void onStop() override;

	// Notification Management
	void addNotification(const std::string& title, const std::string& message, const std::string& appName = "System", const void* icon = nullptr, int priority = 1, bool dismissable = true, const std::string& customId = "", const std::string& redirectAppId = "", const std::string& redirectData = "");
	void removeNotification(const std::string& id);
	void clearAll(bool force = false);
	void markAsRead(const std::string& id);
	void markAllAsRead();

	// Getters
	std::vector<Notification> getNotifications() const;
	size_t getUnreadCount() const;
	bool tryGetLatestNotification(Notification& out) const;

	// Observables for UI binding
	flx::Observable<int32_t>& getUnreadCountObservable() { return m_unread_count_subject; }
	flx::Observable<int32_t>& getUpdateObservable() { return m_update_subject; }
	flx::Observable<int32_t>& getLatestNotificationObservable() { return m_latest_notification_subject; }

private:

	NotificationManager();
	~NotificationManager() = default;

	std::vector<Notification> m_notifications {};
	Notification m_latest_notification {};
	int32_t m_latest_notification_serial {0};
	mutable std::mutex m_mutex {};
	flx::core::EventBus::SubscriptionId m_event_sub_id {0};
	flx::core::EventBus::SubscriptionId m_event_remove_sub_id {0};
	flx::Observable<int32_t> m_unread_count_subject {0};
	flx::Observable<int32_t> m_update_subject {0};
	flx::Observable<int32_t> m_latest_notification_subject {0};

	std::string generateId();
	void updateSubjects();
};

} // namespace flx::system
