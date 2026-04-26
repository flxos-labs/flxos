#include <driver/i2c_master.h>
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
	std::lock_guard<std::recursive_mutex> lock(m_lock);
	this->setState(State::Starting);

	if (m_busHandle != nullptr) {
		this->setState(State::Ready);
		return true;
	}

	i2c_master_bus_config_t conf = {};
	conf.clk_source = I2C_CLK_SRC_DEFAULT;
	conf.i2c_port = static_cast<i2c_port_num_t>(m_port);
	conf.sda_io_num = static_cast<gpio_num_t>(m_sdaPin);
	conf.scl_io_num = static_cast<gpio_num_t>(m_sclPin);
	conf.glitch_ignore_cnt = 7;
	conf.flags.enable_internal_pullup = true;

	esp_err_t err = i2c_new_master_bus(&conf, &m_busHandle);
	if (err != ESP_OK) {
		flx::Log::error(TAG, "Failed to initialize I2C master bus on port %d: %s", m_port, esp_err_to_name(err));
		this->setState(State::Error);
		return false;
	}

	flx::Log::info(TAG, "I2C master bus %d initialized successfully", m_port);
	m_initialized = true;

	this->setState(State::Ready);
	return true;
}

bool EspI2cBus::stop() {
	std::lock_guard<std::recursive_mutex> lock(m_lock);

	if (m_busHandle != nullptr) {
		clearDeviceHandles();

		esp_err_t err = i2c_del_master_bus(m_busHandle);
		if (err != ESP_OK) {
			flx::Log::error(TAG, "Failed to delete I2C master bus %d: %s", m_port, esp_err_to_name(err));
			this->setState(State::Error);
			return false;
		}

		m_busHandle = nullptr;
		flx::Log::info(TAG, "I2C master bus %d deleted", m_port);
	}

	m_initialized = false;
	this->setState(State::Stopped);
	return true;
}

i2c_master_dev_handle_t EspI2cBus::getOrCreateDeviceHandle(uint8_t addr) {
	auto it = m_deviceHandles.find(addr);
	if (it != m_deviceHandles.end()) {
		return it->second;
	}

	i2c_device_config_t devConfig = {};
	devConfig.dev_addr_length = I2C_ADDR_BIT_LEN_7;
	devConfig.device_address = addr;
	devConfig.scl_speed_hz = m_freqHz;

	i2c_master_dev_handle_t handle = nullptr;
	esp_err_t err = i2c_master_bus_add_device(m_busHandle, &devConfig, &handle);
	if (err != ESP_OK) {
		flx::Log::error(TAG, "Failed to add I2C device 0x%02X on bus %d: %s", addr, m_port, esp_err_to_name(err));
		return nullptr;
	}

	m_deviceHandles.emplace(addr, handle);
	return handle;
}

void EspI2cBus::clearDeviceHandles() {
	for (auto& [addr, handle]: m_deviceHandles) {
		esp_err_t err = i2c_master_bus_rm_device(handle);
		if (err != ESP_OK) {
			flx::Log::warn(TAG, "Failed to remove I2C device 0x%02X from bus %d: %s", addr, m_port, esp_err_to_name(err));
		}
	}
	m_deviceHandles.clear();
}

bool EspI2cBus::read(uint8_t addr, uint8_t* data, size_t len, uint32_t timeoutMs) {
	std::lock_guard<std::recursive_mutex> lock(m_lock);

	if (m_busHandle == nullptr) {
		flx::Log::error(TAG, "I2C bus %d is not initialized", m_port);
		return false;
	}
	if (len == 0) {
		return true;
	}
	if (data == nullptr) {
		return false;
	}

	i2c_master_dev_handle_t device = getOrCreateDeviceHandle(addr);
	if (device == nullptr) {
		return false;
	}

	esp_err_t err = i2c_master_receive(device, data, len, static_cast<int>(timeoutMs));

	return err == ESP_OK;
}

bool EspI2cBus::write(uint8_t addr, const uint8_t* data, size_t len, uint32_t timeoutMs) {
	std::lock_guard<std::recursive_mutex> lock(m_lock);

	if (m_busHandle == nullptr) {
		flx::Log::error(TAG, "I2C bus %d is not initialized", m_port);
		return false;
	}
	if (len == 0) {
		return true;
	}
	if (data == nullptr) {
		return false;
	}

	i2c_master_dev_handle_t device = getOrCreateDeviceHandle(addr);
	if (device == nullptr) {
		return false;
	}

	esp_err_t err = i2c_master_transmit(device, data, len, static_cast<int>(timeoutMs));

	return err == ESP_OK;
}

bool EspI2cBus::writeRead(uint8_t addr, const uint8_t* writeData, size_t writeLen, uint8_t* readData, size_t readLen, uint32_t timeoutMs) {
	std::lock_guard<std::recursive_mutex> lock(m_lock);

	if (m_busHandle == nullptr) {
		flx::Log::error(TAG, "I2C bus %d is not initialized", m_port);
		return false;
	}
	if ((writeLen > 0 && writeData == nullptr) || (readLen > 0 && readData == nullptr)) {
		return false;
	}

	i2c_master_dev_handle_t device = getOrCreateDeviceHandle(addr);
	if (device == nullptr) {
		return false;
	}

	esp_err_t err = i2c_master_transmit_receive(device, writeData, writeLen, readData, readLen, static_cast<int>(timeoutMs));
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

	if (m_busHandle == nullptr) {
		flx::Log::error(TAG, "I2C bus %d is not initialized", m_port);
		return foundDevices;
	}

	for (uint8_t addr = 0x03; addr < 0x78; addr++) {
		esp_err_t err = i2c_master_probe(m_busHandle, addr, static_cast<int>(timeoutMs));
		if (err == ESP_OK) {
			foundDevices.push_back(addr);
			(void)getOrCreateDeviceHandle(addr);
		}
	}

	return foundDevices;
}

} // namespace flx::hal::i2c
