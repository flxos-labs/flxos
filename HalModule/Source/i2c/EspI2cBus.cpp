#include <driver/i2c.h>
#include <flx/core/Logger.hpp>
#include <flx/hal/i2c/EspI2cBus.hpp>

namespace flx::hal::i2c {

static constexpr std::string_view TAG = "EspI2cBus";

EspI2cBus::EspI2cBus(int port, int sdaPin, int sclPin, uint32_t freqHz)
	: m_port(port), m_sdaPin(sdaPin), m_sclPin(sclPin), m_freqHz(freqHz) {
	this->setState(State::Uninitialized);
}

EspI2cBus::~EspI2cBus() {
	if (getState() == State::Ready) {
		stop();
	}
}

std::string_view EspI2cBus::getName() const {
	return "ESP I2C Bus";
}

std::string_view EspI2cBus::getDescription() const {
	return "ESP-IDF I2C Master Bus Controller";
}

bool EspI2cBus::start() {
	this->setState(State::Starting);

	i2c_config_t conf = {};
	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = m_sdaPin;
	conf.scl_io_num = m_sclPin;
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	conf.master.clk_speed = m_freqHz;

	i2c_port_t i2c_port = static_cast<i2c_port_t>(m_port);

	esp_err_t err = i2c_param_config(i2c_port, &conf);
	if (err != ESP_OK) {
		flx::Log::error(TAG, "Failed to configure I2C port %d: %s", m_port, esp_err_to_name(err));
		this->setState(State::Error);
		return false;
	}

	err = i2c_driver_install(i2c_port, conf.mode, 0, 0, 0);
	if (err == ESP_ERR_INVALID_STATE) {
		flx::Log::info(TAG, "I2C port %d already installed (shared usage)", m_port);
		m_initialized = false;
	} else if (err != ESP_OK) {
		flx::Log::error(TAG, "Failed to install I2C driver on port %d: %s", m_port, esp_err_to_name(err));
		this->setState(State::Error);
		return false;
	} else {
		flx::Log::info(TAG, "I2C port %d initialized successfully", m_port);
		m_initialized = true;
	}

	this->setState(State::Ready);
	return true;
}

bool EspI2cBus::stop() {
	if (m_initialized) {
		i2c_driver_delete(static_cast<i2c_port_t>(m_port));
		m_initialized = false;
		flx::Log::info(TAG, "I2C port %d driver deleted", m_port);
	}
	this->setState(State::Stopped);
	return true;
}

bool EspI2cBus::read(uint8_t addr, uint8_t* data, size_t len, uint32_t timeoutMs) {
	std::lock_guard<std::recursive_mutex> lock(m_lock);
	i2c_port_t port = static_cast<i2c_port_t>(m_port);

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);
	if (len > 1) {
		i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
	}
	i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
	i2c_master_stop(cmd);

	esp_err_t err = i2c_master_cmd_begin(port, cmd, pdMS_TO_TICKS(timeoutMs));
	i2c_cmd_link_delete(cmd);

	return err == ESP_OK;
}

bool EspI2cBus::write(uint8_t addr, const uint8_t* data, size_t len, uint32_t timeoutMs) {
	std::lock_guard<std::recursive_mutex> lock(m_lock);
	i2c_port_t port = static_cast<i2c_port_t>(m_port);

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
	i2c_master_write(cmd, data, len, true);
	i2c_master_stop(cmd);

	esp_err_t err = i2c_master_cmd_begin(port, cmd, pdMS_TO_TICKS(timeoutMs));
	i2c_cmd_link_delete(cmd);

	return err == ESP_OK;
}

bool EspI2cBus::writeRead(uint8_t addr, const uint8_t* writeData, size_t writeLen, uint8_t* readData, size_t readLen, uint32_t timeoutMs) {
	std::lock_guard<std::recursive_mutex> lock(m_lock);
	i2c_port_t port = static_cast<i2c_port_t>(m_port);

	esp_err_t err = i2c_master_write_read_device(port, addr, writeData, writeLen, readData, readLen, pdMS_TO_TICKS(timeoutMs));
	return err == ESP_OK;
}

bool EspI2cBus::readRegister8(uint8_t addr, uint8_t reg, uint8_t& value) {
	return writeRead(addr, &reg, 1, &value, 1, 100);
}

bool EspI2cBus::writeRegister8(uint8_t addr, uint8_t reg, uint8_t value) {
	uint8_t data[2] = {reg, value};
	return write(addr, data, 2, 100);
}

bool EspI2cBus::readRegister16(uint8_t addr, uint8_t reg, uint16_t& value) {
	uint8_t data[2];
	if (writeRead(addr, &reg, 1, data, 2, 100)) {
		value = (data[0] << 8) | data[1];
		return true;
	}
	return false;
}

bool EspI2cBus::writeRegister16(uint8_t addr, uint8_t reg, uint16_t value) {
	uint8_t data[3] = {reg, static_cast<uint8_t>(value >> 8), static_cast<uint8_t>(value & 0xFF)};
	return write(addr, data, 3, 100);
}

std::vector<uint8_t> EspI2cBus::scan(uint32_t timeoutMs) {
	std::lock_guard<std::recursive_mutex> lock(m_lock);
	std::vector<uint8_t> foundDevices;
	i2c_port_t port = static_cast<i2c_port_t>(m_port);

	for (uint8_t addr = 0x03; addr < 0x78; addr++) {
		i2c_cmd_handle_t cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
		i2c_master_stop(cmd);

		esp_err_t err = i2c_master_cmd_begin(port, cmd, pdMS_TO_TICKS(timeoutMs));
		i2c_cmd_link_delete(cmd);

		if (err == ESP_OK) {
			foundDevices.push_back(addr);
		}
	}

	return foundDevices;
}

} // namespace flx::hal::i2c
