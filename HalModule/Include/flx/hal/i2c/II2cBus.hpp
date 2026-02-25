#pragma once

#include <cstdint>
#include <flx/hal/IDevice.hpp>
#include <mutex>
#include <vector>

namespace flx::hal::i2c {

/**
 * @brief Abstract interface for an I²C bus controller.
 *
 * Key improvements over Tactility's I2c namespace:
 *  - Object-oriented with DeviceRegistry registration
 *  - Real bus locking (Tactility's getLock() returns a NoLock!)
 *  - Bus scanning (enumerate connected device addresses)
 *  - Transaction batching via writeRead()
 *  - Convenience register read/write helpers
 *
 * Usage:
 *   auto bus = DeviceRegistry::getInstance().findFirst<II2cBus>(Type::I2c);
 *   uint8_t val;
 *   bus->readRegister8(0x34, 0x01, val);
 */
class II2cBus : public flx::hal::IDevice {
public:

	// ── IDevice ───────────────────────────────────────────────────────────
	Type getType() const override { return Type::I2c; }

	// ── Raw transfer ──────────────────────────────────────────────────────
	/**
     * @brief Read from an I²C device address.
     * @param addr       7-bit I²C address.
     * @param data       Output buffer.
     * @param len        Bytes to read.
     * @param timeoutMs  Timeout in milliseconds.
     * @return true if read succeeded (ACK received).
     */
	virtual bool read(uint8_t addr, uint8_t* data, size_t len, uint32_t timeoutMs = 100) = 0;

	/**
     * @brief Write to an I²C device address.
     * @param addr       7-bit I²C address.
     * @param data       Data to write.
     * @param len        Bytes to write.
     * @param timeoutMs  Timeout in milliseconds.
     * @return true if write succeeded (ACK received).
     */
	virtual bool write(uint8_t addr, const uint8_t* data, size_t len, uint32_t timeoutMs = 100) = 0;

	/**
     * @brief Write then read (single transaction — avoids bus arbitration issues).
     * Commonly used for register reads: write [reg_addr], read [value].
     */
	virtual bool writeRead(uint8_t addr, const uint8_t* writeData, size_t writeLen, uint8_t* readData, size_t readLen, uint32_t timeoutMs = 100) = 0;

	// ── Register helpers ──────────────────────────────────────────────────
	virtual bool readRegister8(uint8_t addr, uint8_t reg, uint8_t& value) = 0;
	virtual bool writeRegister8(uint8_t addr, uint8_t reg, uint8_t value) = 0;
	virtual bool readRegister16(uint8_t addr, uint8_t reg, uint16_t& value) = 0;
	virtual bool writeRegister16(uint8_t addr, uint8_t reg, uint16_t value) = 0;

	// ── Bus scanning (surpasses Tactility) ────────────────────────────────
	/**
     * @brief Probe all I²C addresses and return responding ones.
     * Scans the standard range 0x03–0x77.
     * @param timeoutMs  Per-address timeout (short values recommended, e.g. 10ms).
     * @return Vector of 7-bit addresses that responded with ACK.
     */
	virtual std::vector<uint8_t> scan(uint32_t timeoutMs = 10) = 0;

	// ── Bus locking (surpasses Tactility) ─────────────────────────────────
	/**
     * @brief Access the bus mutex for RAII locking.
     * Usage: std::lock_guard<std::recursive_mutex> lock(bus.getLock());
     * Tactility's II2cDevice::getLock() returns a dummy NoLock — unusable.
     */
	virtual std::recursive_mutex& getLock() = 0;

	/**
     * @brief I²C port number (0 = I2C0, 1 = I2C1).
     */
	virtual int getPort() const = 0;
};

} // namespace flx::hal::i2c
