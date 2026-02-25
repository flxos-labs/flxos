#pragma once

#include <cstdint>
#include <flx/hal/IDevice.hpp>
#include <mutex>

namespace flx::hal::spi {

/**
 * @brief Abstract interface for an SPI bus controller.
 *
 * Surpasses Tactility — Tactility bridges SPI locking through
 * a C kernel spi_controller_try_lock() shim. FlxOS uses pure C++
 * with a centralized contention manager.
 *
 * Usage:
 *   auto& bus = DeviceRegistry::getInstance().findFirst<ISpiBus>(Type::Spi);
 *   ISpiBus::ScopedLock lock(*bus);   // Acquire exclusive access
 *   // ... do SPI transfer ...
 *   // lock released at end of scope
 */
class ISpiBus : public flx::hal::IDevice {
public:

	// ── IDevice ───────────────────────────────────────────────────────────
	Type getType() const override { return Type::Spi; }

	virtual int getHostId() const = 0;
	virtual bool isShared() const = 0; ///< True if multiple devices share this bus

	// ── Bus locking ───────────────────────────────────────────────────────
	/**
     * @brief Acquire exclusive access to this SPI bus.
     * @param timeoutMs  Maximum wait time in milliseconds.
     * @return true if acquired within timeout.
     */
	virtual bool acquire(uint32_t timeoutMs = 1000) = 0;

	/**
     * @brief Release exclusive access to this SPI bus.
     */
	virtual void release() = 0;

	/**
     * @brief RAII scoped bus lock.
     * Guarantees release on scope exit even on exception/early return.
     *
     * Usage:
     *   {
     *     ISpiBus::ScopedLock lock(bus);
     *     // ... exclusive SPI access here ...
     *   }  // bus released automatically
     */
	struct ScopedLock {
		ISpiBus& bus;
		explicit ScopedLock(ISpiBus& b) : bus(b) { bus.acquire(); }
		~ScopedLock() { bus.release(); }
		ScopedLock(const ScopedLock&) = delete;
		ScopedLock& operator=(const ScopedLock&) = delete;
	};

	// ── Bus statistics (surpasses Tactility) ──────────────────────────────
	virtual uint32_t getTransactionCount() const { return 0; }
	virtual uint32_t getContentionCount() const { return 0; }
};

} // namespace flx::hal::spi
