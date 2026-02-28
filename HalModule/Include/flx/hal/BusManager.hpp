#pragma once

#include <flx/core/Singleton.hpp>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <map>
#include <mutex>

namespace flx::hal {

/**
 * @brief Centralized Bus Contention Manager.
 *
 * Surpasses Tactility's ad-hoc SPI locking by providing a centralized
 * registry of locks for shared buses, particularly SPI where display,
 * touch, and SD card might share the same physical lines.
 */
class BusManager : public flx::Singleton<BusManager> {
	friend class flx::Singleton<BusManager>;

public:

	/**
     * @brief Acquire exclusive access to a shared SPI bus.
     * @param hostId     The SPI host ID (e.g., SPI2_HOST or SPI3_HOST on ESP32).
     * @param timeoutMs  Maximum wait time in milliseconds.
     * @return true if acquired within timeout.
     */
	bool acquireSpi(int hostId, uint32_t timeoutMs = 1000);

	/**
     * @brief Release exclusive access to a shared SPI bus.
     * @param hostId     The SPI host ID.
     */
	void releaseSpi(int hostId);

	/**
     * @brief RAII scoped bus lock for centralized contention management.
     */
	class ScopedBusLock {
		int m_hostId;
		bool m_acquired;

	public:

		explicit ScopedBusLock(int hostId, uint32_t timeoutMs = 1000) : m_hostId(hostId) {
			m_acquired = BusManager::getInstance().acquireSpi(hostId, timeoutMs);
		}
		~ScopedBusLock() {
			if (m_acquired) {
				BusManager::getInstance().releaseSpi(m_hostId);
			}
		}

		bool isAcquired() const { return m_acquired; }
		ScopedBusLock(const ScopedBusLock&) = delete;
		ScopedBusLock& operator=(const ScopedBusLock&) = delete;
	};

private:

	BusManager() = default;
	~BusManager();

	std::mutex m_managerMutex; // Protects the map
	std::map<int, SemaphoreHandle_t> m_spiLocks;
};

} // namespace flx::hal
