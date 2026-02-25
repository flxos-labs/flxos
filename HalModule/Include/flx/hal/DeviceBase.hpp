#pragma once

#include "IDevice.hpp"
#include <atomic>

namespace flx::hal {

/**
 * @brief Helper for generating unique device IDs.
 */
IDevice::Id _generateDeviceId();

/**
 * @brief Convenience base class for concrete device implementations.
 *
 * Provides:
 *  - Auto-incrementing unique ID (thread-safe, global counter)
 *  - Atomic state tracking (safe for cross-task reads)
 *
 * All concrete HAL devices inherit from this. By using CRTP, the
 * concrete class implements exactly one interface hierarchy without
 * multiple inheritance of IDevice, which is important because RTTI
 * is disabled in ESP-IDF (-fno-rtti).
 *
 * Usage:
 *   class MyDevice : public DeviceBase<IMyDevice> { ... };
 */
template<typename BaseInterface = IDevice>
class DeviceBase : public BaseInterface {
public:

	DeviceBase() : m_id(_generateDeviceId()), m_state(IDevice::State::Uninitialized) {}
	~DeviceBase() override = default;

	// ── IDevice identity ──
	IDevice::Id getId() const override { return m_id; }
	IDevice::State getState() const override { return m_state.load(std::memory_order_acquire); }

protected:

	/**
     * @brief Update device state (called by concrete implementations).
     * Thread-safe — uses atomic store.
     */
	void setState(IDevice::State s) { m_state.store(s, std::memory_order_release); }

private:

	const IDevice::Id m_id; ///< Immutable, assigned at construction
	std::atomic<IDevice::State> m_state; ///< Current lifecycle state
};

} // namespace flx::hal
