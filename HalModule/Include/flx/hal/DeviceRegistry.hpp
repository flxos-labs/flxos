#pragma once

#include "IDevice.hpp"
#include <flx/core/Singleton.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <string_view>
#include <vector>

namespace flx::hal {

/**
 * @brief Callback invoked when a device is added or removed from the registry.
 *
 * Surpasses Tactility — Tactility has no equivalent observer pattern on devices.
 *
 * @param device  The device that was added or removed.
 * @param added   true if device was registered, false if deregistered.
 */
using DeviceChangeCallback = std::function<void(const std::shared_ptr<IDevice>&, bool /*added*/)>;

/**
 * @brief Thread-safe, observable central registry for all HAL devices.
 *
 * Key improvements over Tactility:
 *  - Observable: subscribe to device add/remove events
 *  - Health reporting: centralized device health aggregation
 *  - Thread-safe: recursive_mutex handles nested lock scenarios
 *  - No C kernel shim: pure C++ (Tactility bridges to device_for_each_of_type())
 *  - Type-safe queries: findFirst<IDisplayDevice>(Type::Display)
 *
 * Usage:
 *   auto& reg = DeviceRegistry::getInstance();
 *   reg.registerDevice(std::make_shared<LgfxDisplayDevice>());
 *   auto display = reg.findFirst<IDisplayDevice>(IDevice::Type::Display);
 */
class DeviceRegistry : public flx::Singleton<DeviceRegistry> {
	friend class flx::Singleton<DeviceRegistry>;

public:

	// ── Registration ──────────────────────────────────────────────────────

	/**
     * @brief Register a device with the registry.
     * Notifies all observers. Thread-safe.
     */
	void registerDevice(std::shared_ptr<IDevice> device);

	/**
     * @brief Remove a device by ID.
     * Notifies all observers. Thread-safe.
     */
	void deregisterDevice(IDevice::Id id);

	// ── Queries ───────────────────────────────────────────────────────────

	std::shared_ptr<IDevice> findById(IDevice::Id id) const;
	std::shared_ptr<IDevice> findByName(std::string_view name) const;
	std::vector<std::shared_ptr<IDevice>> findByType(IDevice::Type type) const;
	std::vector<std::shared_ptr<IDevice>> getAll() const;

	bool hasDevice(IDevice::Type type) const;
	size_t count() const;

	/**
     * @brief Find the first registered device of the given type, cast to T.
     *
     * Example:
     *   auto display = registry.findFirst<IDisplayDevice>(Type::Display);
     *
     * @tparam T  Concrete or abstract interface type to cast to.
     * @return    Shared pointer to T, or nullptr if not found.
     */
	template<typename T>
	std::shared_ptr<T> findFirst(IDevice::Type type) const {
		auto devices = findByType(type);
		for (auto& dev: devices) {
			if (auto typed = std::dynamic_pointer_cast<T>(dev)) {
				return typed;
			}
		}
		return nullptr;
	}

	/**
     * @brief Find all registered devices of the given type, cast to T.
     *
     * @tparam T  Concrete or abstract interface type to cast to.
     * @return    Vector of matching devices (may be empty).
     */
	template<typename T>
	std::vector<std::shared_ptr<T>> findAll(IDevice::Type type) const {
		std::vector<std::shared_ptr<T>> result;
		auto devices = findByType(type);
		for (auto& dev: devices) {
			if (auto typed = std::dynamic_pointer_cast<T>(dev)) {
				result.push_back(typed);
			}
		}
		return result;
	}

	// ── Observers ─────────────────────────────────────────────────────────

	/**
     * @brief Subscribe to device add/remove events.
     * Surpasses Tactility — no equivalent exists.
     *
     * @param callback  Function called when a device is registered/deregistered.
     * @return          Subscription ID (use with unsubscribe()).
     */
	int subscribe(DeviceChangeCallback callback);

	/**
     * @brief Unsubscribe from device change events.
     * @param subscriptionId  ID returned by subscribe().
     */
	void unsubscribe(int subscriptionId);

	// ── Health ────────────────────────────────────────────────────────────

	/**
     * @brief Aggregated health report across all registered devices.
     * Surpasses Tactility — no equivalent health query API.
     */
	struct HealthReport {
		size_t totalDevices = 0;
		size_t healthyDevices = 0;
		size_t errorDevices = 0;
		std::vector<std::pair<IDevice::Id, IDevice::State>> unhealthyDevices;
	};

	HealthReport getHealthReport() const;

	// ── Debug ─────────────────────────────────────────────────────────────

	/**
     * @brief Log all registered devices to the console via flx::Log.
     * Used by the `hal devices` CLI command.
     */
	void dumpDevices() const;

private:

	DeviceRegistry() = default;

	mutable std::recursive_mutex m_mutex;
	std::vector<std::shared_ptr<IDevice>> m_devices;
	std::vector<std::pair<int, DeviceChangeCallback>> m_observers;
	int m_nextSubscriptionId = 0;

	void notifyObservers(const std::shared_ptr<IDevice>& device, bool added);
};

} // namespace flx::hal
