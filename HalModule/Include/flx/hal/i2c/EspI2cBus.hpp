#pragma once

#include <flx/hal/DeviceBase.hpp>
#include <flx/hal/i2c/II2cBus.hpp>
#include <string_view>

namespace flx::hal::i2c {

class EspI2cBus : public DeviceBase<II2cBus> {
public:

	EspI2cBus(int port, int sdaPin, int sclPin, uint32_t freqHz);
	~EspI2cBus() override;

	// ── IDevice ───────────────────────────────────────────────────────────
	std::string_view getName() const override;
	std::string_view getDescription() const override;
	bool start() override;
	bool stop() override;
	Type getType() const override { return II2cBus::getType(); }

	// ── II2cBus ───────────────────────────────────────────────────────────
	bool read(uint8_t addr, uint8_t* data, size_t len, uint32_t timeoutMs = 100) override;
	bool write(uint8_t addr, const uint8_t* data, size_t len, uint32_t timeoutMs = 100) override;
	bool writeRead(uint8_t addr, const uint8_t* writeData, size_t writeLen, uint8_t* readData, size_t readLen, uint32_t timeoutMs = 100) override;

	bool readRegister8(uint8_t addr, uint8_t reg, uint8_t& value) override;
	bool writeRegister8(uint8_t addr, uint8_t reg, uint8_t value) override;
	bool readRegister16(uint8_t addr, uint8_t reg, uint16_t& value) override;
	bool writeRegister16(uint8_t addr, uint8_t reg, uint16_t value) override;

	std::vector<uint8_t> scan(uint32_t timeoutMs = 10) override;
	std::recursive_mutex& getLock() override { return m_lock; }
	int getPort() const override { return m_port; }

private:

	int m_port;
	int m_sdaPin;
	int m_sclPin;
	uint32_t m_freqHz;
	std::recursive_mutex m_lock;
	bool m_initialized = false;
};

} // namespace flx::hal::i2c
