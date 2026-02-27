#include <flx/hal/spi/EspSpiBus.hpp>
#include <flx/hal/BusManager.hpp>
#include <flx/core/Logger.hpp>
#include <driver/spi_common.h>

namespace flx::hal::spi {

static constexpr std::string_view TAG = "EspSpiBus";

EspSpiBus::EspSpiBus(int hostId, int miso, int mosi, int sclk, int maxTransferSz)
    : m_hostId(hostId), m_miso(miso), m_mosi(mosi), m_sclk(sclk), m_maxTransferSz(maxTransferSz) {
	this->setState(State::Uninitialized);
}

EspSpiBus::~EspSpiBus() {
	if (getState() == State::Ready) {
		stop();
	}
}

std::string_view EspSpiBus::getName() const {
	return "ESP SPI Bus";
}

std::string_view EspSpiBus::getDescription() const {
	return "ESP-IDF SPI Master Bus Controller";
}

bool EspSpiBus::start() {
	this->setState(State::Starting);

	spi_bus_config_t bus_cfg = {};
	bus_cfg.mosi_io_num = m_mosi;
	bus_cfg.miso_io_num = m_miso;
	bus_cfg.sclk_io_num = m_sclk;
	bus_cfg.quadwp_io_num = -1;
	bus_cfg.quadhd_io_num = -1;
	bus_cfg.max_transfer_sz = m_maxTransferSz;

	const spi_host_device_t host_device = static_cast<spi_host_device_t>(m_hostId);
	const esp_err_t bus_ret = spi_bus_initialize(host_device, &bus_cfg, SPI_DMA_CH_AUTO);

	if (bus_ret == ESP_OK) {
		flx::Log::info(TAG, "SPI bus %d initialized successfully", m_hostId);
		m_initialized = true;
	} else if (bus_ret == ESP_ERR_INVALID_STATE) {
		flx::Log::info(TAG, "SPI bus %d already initialized (shared usage)", m_hostId);
		m_initialized = false; // We don't own it, so we won't free it
	} else {
		flx::Log::error(TAG, "Failed to initialize SPI bus %d: %s", m_hostId, esp_err_to_name(bus_ret));
		this->setState(State::Error);
		return false;
	}

	this->setState(State::Ready);
	return true;
}

bool EspSpiBus::stop() {
	if (m_initialized) {
		const spi_host_device_t host_device = static_cast<spi_host_device_t>(m_hostId);
		const esp_err_t bus_ret = spi_bus_free(host_device);
		if (bus_ret != ESP_OK) {
			flx::Log::warn(TAG, "Failed to free SPI bus %d: %s", m_hostId, esp_err_to_name(bus_ret));
		} else {
			flx::Log::info(TAG, "SPI bus %d released", m_hostId);
		}
		m_initialized = false;
	}

	this->setState(State::Stopped);
	return true;
}

bool EspSpiBus::acquire(uint32_t timeoutMs) {
	if (BusManager::getInstance().acquireSpi(m_hostId, timeoutMs)) {
		m_transactionCount++;
		return true;
	}
	m_contentionCount++;
	return false;
}

void EspSpiBus::release() {
	BusManager::getInstance().releaseSpi(m_hostId);
}

} // namespace flx::hal::spi
