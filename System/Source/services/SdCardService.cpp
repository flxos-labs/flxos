#include "sdkconfig.h"
#include <Config.hpp>
#include <flx/core/Logger.hpp>
#include <flx/system/services/SdCardService.hpp>
#include <string_view>

#if FLXOS_SD_CARD_ENABLED
#include "driver/gpio.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#endif

namespace flx::services {

static constexpr std::string_view TAG = "SdCard";

const ServiceManifest SdCardService::serviceManifest = {
	.serviceId = "com.flxos.sdcard",
	.serviceName = "SD Card",
	.dependencies = {},
	.priority = 15,
	.required = false,
	.autoStart = true,
	.guiRequired = false,
	.capabilities = ServiceCapability::Storage,
	.description = "SD card mount/unmount via SPI",
};

SdCardService::SdCardService()
	: m_mountPoint(flx::config::sdcard.mountPoint) {}

SdCardService& SdCardService::getInstance() {
	static SdCardService instance;
	return instance;
}

bool SdCardService::onStart() { return mount(); }
void SdCardService::onGuiInit() {}
void SdCardService::onStop() { unmount(); }

bool SdCardService::mount() {
#if !FLXOS_SD_CARD_ENABLED
	Log::info(TAG, "SD card support disabled in config");
	return false;
#else
	if (m_mounted) {
		Log::warn(TAG, "SD card already mounted");
		return true;
	}

	Log::info(TAG, "Mounting SD card at %s...", m_mountPoint.c_str());

	const esp_vfs_fat_sdmmc_mount_config_t mount_config = {
		.format_if_mount_failed = false,
		.max_files = 5,
		.allocation_unit_size = 16 * 1024,
		.disk_status_check_enable = false,
		.use_one_fat = false,
	};

	m_card = nullptr;
	const spi_host_device_t host_id = flx::config::sdcard.spiHost;

	// Validate pin config is compile-time constant as expected
	static_assert(
		flx::config::sdcard.pins.mosi == flx::config::sdcard.pins.mosi,
		"SD card pin config must be constexpr"
	);

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
		Log::info(TAG, "SPI bus initialized by SdCardService");
		m_spiOwner = true; // <-- track ownership so we can free on unmount
	} else if (bus_ret == ESP_ERR_INVALID_STATE) {
		Log::info(TAG, "SPI bus already initialized (shared with display)");
		m_spiOwner = false;
	} else {
		Log::error(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(bus_ret));
		return false;
	}

	sdmmc_host_t host = SDSPI_HOST_DEFAULT();
	host.slot = host_id;
	host.max_freq_khz = flx::config::sdcard.maxFreqKhz;

	sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
	slot_config.gpio_cs = static_cast<gpio_num_t>(flx::config::sdcard.cs);
	slot_config.host_id = host_id;

	Log::info(TAG, "Mounting via SPI host %d, max freq %d kHz", host_id, host.max_freq_khz);

	const esp_err_t ret = esp_vfs_fat_sdspi_mount(
		m_mountPoint.c_str(), &host, &slot_config, &mount_config, &m_card
	);

	if (ret != ESP_OK) {
		Log::error(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
		// Release bus if we own it and mount failed
		if (m_spiOwner) {
			spi_bus_free(host_id);
			m_spiOwner = false;
		}
		return false;
	}

	m_mounted = true;
	Log::info(TAG, "SD card mounted at %s", m_mountPoint.c_str());

	if (m_card) {
		// sdmmc_card_print_info writes to stdout — fine for debug builds,
		// consider a Log::debug wrapper for production
		sdmmc_card_print_info(stdout, m_card);
	}

	return true;
#endif
}

void SdCardService::unmount() {
#if FLXOS_SD_CARD_ENABLED
	if (!m_mounted) {
		return;
	}

	const spi_host_device_t host_id = flx::config::sdcard.spiHost;

	const esp_err_t ret = esp_vfs_fat_sdcard_unmount(m_mountPoint.c_str(), m_card);
	if (ret != ESP_OK) {
		Log::error(TAG, "Failed to unmount SD card: %s", esp_err_to_name(ret));
		return;
	}

	m_card = nullptr;
	m_mounted = false;
	Log::info(TAG, "SD card unmounted");

	// Release SPI bus only if this service initialized it
	if (m_spiOwner) {
		const esp_err_t bus_ret = spi_bus_free(host_id);
		if (bus_ret != ESP_OK) {
			Log::warn(TAG, "Failed to free SPI bus: %s", esp_err_to_name(bus_ret));
		} else {
			Log::info(TAG, "SPI bus released");
		}
		m_spiOwner = false;
	}
#endif
}

} // namespace flx::services
