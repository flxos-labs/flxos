#pragma once

#include <cstdint>
#include <flx/hal/DeviceBase.hpp>
#include <flx/hal/i2c/II2cBus.hpp>
#include <memory>

namespace flx::hal::i2c {

/**
 * @brief Base class for peripherals connected to an I²C bus.
 *
 * All I²C peripheral devices (power ICs, IO expanders, sensors, etc.)
 * inherit from this class to get bus injection, address management,
 * and convenient register I/O helpers.
 *
 * Compared to Tactility's I2cDevice:
 *  - Bus is injected (not hardcoded to a port number)
 *  - Bit manipulation helpers (readBit8, writeBit8, modifyRegister8)
 *  - Type-safe address stored as uint8_t
 *
 * Usage:
 *   class AXP2101 : public I2cPeripheral {
 *       bool start() override {
 *           if (!probeDevice()) { setState(State::Error); return false; }
 *           // ... setup ...
 *           setState(State::Ready);
 *           return true;
 *       }
 *   };
 */
class I2cPeripheral : public DeviceBase {
public:

	/**
     * @brief Construct an I²C peripheral.
     * @param bus      Shared pointer to the bus this peripheral lives on.
     * @param address  7-bit I²C address.
     */
	explicit I2cPeripheral(std::shared_ptr<II2cBus> bus, uint8_t address)
		: m_bus(std::move(bus)), m_address(address) {}

	~I2cPeripheral() override = default;

	uint8_t getAddress() const { return m_address; }

protected:

	std::shared_ptr<II2cBus> m_bus;
	uint8_t m_address;

	// ── Convenience helpers ───────────────────────────────────────────────

	bool readReg8(uint8_t reg, uint8_t& val) {
		return m_bus && m_bus->readRegister8(m_address, reg, val);
	}

	bool writeReg8(uint8_t reg, uint8_t val) {
		return m_bus && m_bus->writeRegister8(m_address, reg, val);
	}

	bool readReg16(uint8_t reg, uint16_t& val) {
		return m_bus && m_bus->readRegister16(m_address, reg, val);
	}

	bool writeReg16(uint8_t reg, uint16_t val) {
		return m_bus && m_bus->writeRegister16(m_address, reg, val);
	}

	/**
     * @brief Read a single bit from a register.
     * @param reg   Register address.
     * @param bit   Bit position (0–7, LSB = 0).
     * @param value Output: true if bit is set.
     * @return true if the register was read successfully.
     */
	bool readBit8(uint8_t reg, uint8_t bit, bool& value) {
		uint8_t v = 0;
		if (!readReg8(reg, v)) return false;
		value = (v >> bit) & 0x01;
		return true;
	}

	/**
     * @brief Write a single bit in a register (read-modify-write).
     */
	bool writeBit8(uint8_t reg, uint8_t bit, bool value) {
		uint8_t v = 0;
		if (!readReg8(reg, v)) return false;
		if (value) v |= (1u << bit);
		else
			v &= ~(1u << bit);
		return writeReg8(reg, v);
	}

	/**
     * @brief Modify a multi-bit field in a register (read-modify-write).
     * @param reg    Register address.
     * @param mask   Bitmask of the field to modify.
     * @param value  New value (will be ANDed with mask before writing).
     */
	bool modifyRegister8(uint8_t reg, uint8_t mask, uint8_t value) {
		uint8_t v = 0;
		if (!readReg8(reg, v)) return false;
		v = (v & ~mask) | (value & mask);
		return writeReg8(reg, v);
	}

	/**
     * @brief Probe the device — check it responds on the bus.
     * @return true if device ACKs its address.
     */
	bool probeDevice() {
		if (!m_bus) return false;
		uint8_t dummy = 0;
		return m_bus->read(m_address, &dummy, 1, 10);
	}
};

} // namespace flx::hal::i2c
