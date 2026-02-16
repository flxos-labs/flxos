#pragma once

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
	void addNotification(const std::string& title, const std::string& message, const std::string& appName = "System", const void* icon = nullptr, int priority = 1);
	void removeNotification(const std::string& id);
	void clearAll();
	void markAsRead(const std::string& id);
	void markAllAsRead();

	// Getters
	std::vector<Notification> getNotifications() const;
	size_t getUnreadCount() const;

	// Observables for UI binding
	flx::Observable<int32_t>& getUnreadCountObservable() { return m_unread_count_subject; }
	flx::Observable<int32_t>& getUpdateObservable() { return m_update_subject; }

private:

	NotificationManager();
	~NotificationManager() = default;

	std::vector<Notification> m_notifications {};
	mutable std::mutex m_mutex {};
	flx::Observable<int32_t> m_unread_count_subject {0};
	flx::Observable<int32_t> m_update_subject {0};

	std::string generateId();
	void updateSubjects();
};

} // namespace flx::system
