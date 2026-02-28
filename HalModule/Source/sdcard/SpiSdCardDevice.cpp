#include "Config.hpp"
#include <flx/core/GuiLockGuard.hpp>
#include <flx/core/Logger.hpp>
#include <flx/hal/BusManager.hpp>
#include <flx/hal/sdcard/SpiSdCardDevice.hpp>

#if FLXOS_SD_CARD_ENABLED
#include "driver/gpio.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#endif

namespace flx::hal::sdcard {

static constexpr std::string_view TAG = "SpiSdCardDevice";

SpiSdCardDevice::SpiSdCardDevice() {
	this->setState(State::Uninitialized);
}

SpiSdCardDevice::~SpiSdCardDevice() {
	if (getState() == State::Ready || m_mountState == MountState::Mounted) {
		stop();
	}
}

bool SpiSdCardDevice::start() {
	this->setState(State::Starting);
#if FLXOS_SD_CARD_ENABLED
	this->setState(State::Ready);
	return true;
#else
	flx::Log::warn(TAG, "SD Card support disabled in config.");
	this->setState(State::Error);
	return false;
#endif
}

bool SpiSdCardDevice::stop() {
#if FLXOS_SD_CARD_ENABLED
	if (m_mountState == MountState::Mounted) {
		unmount();
	}

	if (m_spiOwner) {
		const spi_host_device_t host_id = static_cast<spi_host_device_t>(flx::config::sdcard.spiHost);
		const esp_err_t bus_ret = spi_bus_free(host_id);
		if (bus_ret != ESP_OK) {
			flx::Log::warn(TAG, "Failed to free SPI bus: %s", esp_err_to_name(bus_ret));
		} else {
			flx::Log::info(TAG, "SPI bus released");
		}
		m_spiOwner = false;
	}
#endif
	this->setState(State::Stopped);
	return true;
}

bool SpiSdCardDevice::mount(const std::string& mountPath) {
#if FLXOS_SD_CARD_ENABLED
	flx::core::GuiLockGuard lock;
	flx::hal::BusManager::ScopedBusLock busLock(flx::config::sdcard.spiHost);

	if (m_mountState == MountState::Mounted) {
		flx::Log::warn(TAG, "SD card already mounted");
		return true;
	}

	m_mountPath = mountPath;
	flx::Log::info(TAG, "Mounting SD card at %s...", m_mountPath.c_str());

	const esp_vfs_fat_sdmmc_mount_config_t mount_config = {
		.format_if_mount_failed = false,
		.max_files = 5,
		.allocation_unit_size = 16 * 1024,
		.disk_status_check_enable = false,
		.use_one_fat = false,
	};

	m_card = nullptr;
	const spi_host_device_t host_id = static_cast<spi_host_device_t>(flx::config::sdcard.spiHost);

	spi_bus_config_t bus_cfg = {};
	if constexpr (flx::config::sdcard.pins.mosi != -1) {
		bus_cfg.mosi_io_num = flx::config::sdcard.pins.mosi;
		bus_cfg.miso_io_num = flx::config::sdcard.pins.miso;
		bus_cfg.sclk_io_num = flx::config::sdcard.pins.sclk;
	} else {
		bus_cfg.mosi_io_num = flx::config::display.pins.mosi;
		bus_cfg.miso_io_num = flx::config::display.pins.miso;
		bus_cfg.sclk_io_num = flx::config::display.pins.sclk;
	}
	bus_cfg.quadwp_io_num = -1;
	bus_cfg.quadhd_io_num = -1;
	bus_cfg.max_transfer_sz = 16 * 1024;

	const esp_err_t bus_ret = spi_bus_initialize(host_id, &bus_cfg, SPI_DMA_CH_AUTO);
	if (bus_ret == ESP_OK) {
		flx::Log::info(TAG, "SPI bus initialized by SpiSdCardDevice");
		m_spiOwner = true;
	} else if (bus_ret == ESP_ERR_INVALID_STATE) {
		flx::Log::info(TAG, "SPI bus already initialized (shared)");
		m_spiOwner = false;
	} else {
		flx::Log::error(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(bus_ret));
		return false;
	}

	sdmmc_host_t host = SDSPI_HOST_DEFAULT();
	host.slot = host_id;
	host.max_freq_khz = flx::config::sdcard.maxFreqKhz;

	sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
	slot_config.gpio_cs = static_cast<gpio_num_t>(flx::config::sdcard.cs);
	slot_config.host_id = host_id;

	sdmmc_card_t* card_local = nullptr;
	const esp_err_t ret = esp_vfs_fat_sdspi_mount(
		m_mountPath.c_str(), &host, &slot_config, &mount_config, &card_local
	);

	if (ret != ESP_OK) {
		flx::Log::error(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
		if (m_spiOwner) {
			spi_bus_free(host_id);
			m_spiOwner = false;
		}
		m_mountState = MountState::Error;
		return false;
	}

	m_card = card_local;
	m_mountState = MountState::Mounted;
	flx::Log::info(TAG, "SD card mounted at %s", m_mountPath.c_str());
	return true;
#else
	return false;
#endif
}

bool SpiSdCardDevice::unmount() {
#if FLXOS_SD_CARD_ENABLED
	flx::core::GuiLockGuard lock;
	flx::hal::BusManager::ScopedBusLock busLock(flx::config::sdcard.spiHost);

	if (m_mountState != MountState::Mounted) {
		return false;
	}

	const esp_err_t ret = esp_vfs_fat_sdcard_unmount(m_mountPath.c_str(), static_cast<sdmmc_card_t*>(m_card));
	if (ret != ESP_OK) {
		flx::Log::error(TAG, "Failed to unmount SD card: %s", esp_err_to_name(ret));
		return false;
	}

	m_card = nullptr;
	m_mountState = MountState::Unmounted;
	flx::Log::info(TAG, "SD card unmounted");
	return true;
#else
	return false;
#endif
}

ISdCardDevice::MountState SpiSdCardDevice::getMountState() const {
	return m_mountState;
}

std::string SpiSdCardDevice::getMountPath() const {
	return m_mountPath;
}

std::recursive_mutex& SpiSdCardDevice::getLock() {
	return m_spiLock;
}

bool SpiSdCardDevice::getCardInfo(CardInfo& info) const {
#if FLXOS_SD_CARD_ENABLED
	flx::core::GuiLockGuard lock;
	flx::hal::BusManager::ScopedBusLock busLock(flx::config::sdcard.spiHost);

	if (m_mountState != MountState::Mounted || !m_card) return false;

	auto* card = static_cast<sdmmc_card_t*>(m_card);
	info.totalBytes = static_cast<uint64_t>(card->csd.capacity) * card->csd.sector_size;
	info.freeBytes = 0;
	info.maxFreqKhz = card->max_freq_khz;
	info.fsType = "FAT";

	esp_vfs_fat_info(m_mountPath.c_str(), &info.totalBytes, &info.freeBytes);

	return true;
#else
	return false;
#endif
}

} // namespace flx::hal::sdcard
