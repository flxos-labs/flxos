#pragma once

#include <cstdint>
#include <flx/core/Bundle.hpp>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace flx::core {

/**
 * @brief System-wide publish/subscribe event bus
 *
 * Enables decoupled communication between apps, services, and system components.
 *
 * Standard system events:
 * - "app.started"       — Bundle: { "appId": string }
 * - "app.stopped"       — Bundle: { "appId": string }
 * - "wifi.connected"    — Bundle: { "ssid": string, "ip": string }
 * - "wifi.disconnected" — Bundle: {}
 * - "sdcard.mounted"    — Bundle: { "path": string }
 * - "sdcard.unmounted"  — Bundle: {}
 * - "app.installed"     — Bundle: { "appId": string }
 * - "app.uninstalled"   — Bundle: { "appId": string }
 *
 * Thread-safe: all methods can be called from any task/thread.
 */
class EventBus {
public:

	using SubscriptionId = uint32_t;
	using Callback = std::function<void(const std::string& event, const Bundle& data)>;

	static EventBus& getInstance();

	/**
	 * Subscribe to a specific event.
	 * @param event Event name to listen for (e.g. "wifi.connected")
	 * @param callback Function called when the event is published
	 * @return Subscription ID for later unsubscription
	 */
	SubscriptionId subscribe(const std::string& event, Callback callback);

	/**
	 * Subscribe to all events (wildcard listener).
	 * @param callback Function called for every published event
	 * @return Subscription ID for later unsubscription
	 */
	SubscriptionId subscribeAll(Callback callback);

	/**
	 * Remove a subscription by its ID.
	 */
	void unsubscribe(SubscriptionId id);

	/**
	 * Publish an event to all subscribers.
	 * @param event Event name
	 * @param data Optional event payload
	 */
	void publish(const std::string& event, const Bundle& data = {});

private:

	EventBus() = default;
	~EventBus() = default;
	EventBus(const EventBus&) = delete;
	EventBus& operator=(const EventBus&) = delete;

	struct Subscription {
		SubscriptionId id;
		std::string event; // Empty = wildcard (all events)
		Callback callback;
	};

	std::vector<Subscription> m_subscriptions;
	SubscriptionId m_nextId = 1;
	mutable std::mutex m_mutex;
};

// Standard event name constants
namespace Events {

// App lifecycle events
constexpr const char* APP_STARTED = "app.started";
constexpr const char* APP_STOPPED = "app.stopped";
constexpr const char* APP_INSTALLED = "app.installed";
constexpr const char* APP_UNINSTALLED = "app.uninstalled";
constexpr const char* APP_CRASHED = "app.crashed";
constexpr const char* APP_BLOCKED = "app.blocked";
constexpr const char* APP_RESUMED = "app.resumed";
constexpr const char* APP_PAUSED = "app.paused";

// Service lifecycle events
constexpr const char* SERVICE_STARTED = "service.started";
constexpr const char* SERVICE_STOPPED = "service.stopped";
constexpr const char* SERVICE_FAILED = "service.failed";
constexpr const char* SERVICE_RESTARTED = "service.restarted";
constexpr const char* SERVICE_HEALTH_DEGRADED = "service.health.degraded";

// Hardware / system events
constexpr const char* WIFI_CONNECTED = "wifi.connected";
constexpr const char* WIFI_DISCONNECTED = "wifi.disconnected";
constexpr const char* SDCARD_MOUNTED = "sdcard.mounted";
constexpr const char* SDCARD_UNMOUNTED = "sdcard.unmounted";

} // namespace Events

} // namespace flx::core
// Minor commit 1
