#include "EventBus.hpp"
#include <flx/core/Logger.hpp>

namespace System::Apps {

static constexpr const char* TAG = "EventBus";

EventBus& EventBus::getInstance() {
	static EventBus instance;
	return instance;
}

EventBus::SubscriptionId EventBus::subscribe(const std::string& event, Callback callback) {
	std::lock_guard<std::mutex> lock(m_mutex);
	SubscriptionId id = m_nextId++;
	m_subscriptions.push_back({id, event, std::move(callback)});
	Log::info(TAG, "Subscribed to '%s' (id=%lu)", event.c_str(), (unsigned long)id);
	return id;
}

EventBus::SubscriptionId EventBus::subscribeAll(Callback callback) {
	std::lock_guard<std::mutex> lock(m_mutex);
	SubscriptionId id = m_nextId++;
	m_subscriptions.push_back({id, "", std::move(callback)});
	Log::info(TAG, "Subscribed to all events (id=%lu)", (unsigned long)id);
	return id;
}

void EventBus::unsubscribe(SubscriptionId id) {
	std::lock_guard<std::mutex> lock(m_mutex);
	for (auto it = m_subscriptions.begin(); it != m_subscriptions.end(); ++it) {
		if (it->id == id) {
			Log::info(TAG, "Unsubscribed id=%lu", (unsigned long)id);
			m_subscriptions.erase(it);
			return;
		}
	}
}

void EventBus::publish(const std::string& event, const Bundle& data) {
	// Copy subscriptions under lock, then invoke outside lock to avoid deadlocks
	std::vector<Callback> toNotify;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		for (const auto& sub: m_subscriptions) {
			if (sub.event.empty() || sub.event == event) {
				toNotify.push_back(sub.callback);
			}
		}
	}

	for (const auto& cb: toNotify) {
		if (cb) {
			cb(event, data);
		}
	}
}

} // namespace System::Apps
