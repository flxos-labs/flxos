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

static constexpr std::string_view TAG = "SdCard";

namespace flx::services {

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
	: m_mountPoint(flx::config::sdcard.mountPoint) {
}

SdCardService& SdCardService::getInstance() {
	static SdCardService instance;
	return instance;
}

bool SdCardService::onStart() {
	return mount();
}

void SdCardService::onGuiInit() {
}

void SdCardService::onStop() {
	unmount();
}

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

	esp_vfs_fat_sdmmc_mount_config_t mount_config = {
		.format_if_mount_failed = false,
		.max_files = 5,
		.allocation_unit_size = 16 * 1024,
		.disk_status_check_enable = false,
		.use_one_fat = false,
	};

	m_card = nullptr;

	auto host_id = flx::config::sdcard.spiHost;

	// Initialize SPI bus (idempotent — returns ESP_ERR_INVALID_STATE if LGFX already did it)
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

	esp_err_t bus_ret = spi_bus_initialize(host_id, &bus_cfg, SPI_DMA_CH_AUTO);
	if (bus_ret == ESP_OK) {
		Log::info(TAG, "SPI bus initialized by SdCardService");
	} else if (bus_ret == ESP_ERR_INVALID_STATE) {
		Log::info(TAG, "SPI bus already initialized (shared with display)");
	} else {
		Log::error(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(bus_ret));
		return false;
	}

	sdmmc_host_t host = SDSPI_HOST_DEFAULT();
	host.slot = host_id;
	host.max_freq_khz = flx::config::sdcard.maxFreqKhz;

	sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
	slot_config.gpio_cs = (gpio_num_t)flx::config::sdcard.cs;
	slot_config.host_id = host_id;

	Log::info(TAG, "Using shared SPI bus (host_id=%d) for SD card. Max freq: %d kHz", host_id, host.max_freq_khz);

	Log::info(TAG, "Calling esp_vfs_fat_sdspi_mount...");
	esp_err_t ret = esp_vfs_fat_sdspi_mount(
		m_mountPoint.c_str(), &host, &slot_config, &mount_config, &m_card
	);
	Log::info(TAG, "esp_vfs_fat_sdspi_mount returned: %s", esp_err_to_name(ret));

	if (ret != ESP_OK) {
		Log::error(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
		return false;
	}

	m_mounted = true;
	Log::info(TAG, "SD card mounted at %s", m_mountPoint.c_str());
	if (m_card) {
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

	esp_err_t ret = esp_vfs_fat_sdcard_unmount(m_mountPoint.c_str(), m_card);
	if (ret != ESP_OK) {
		Log::error(TAG, "Failed to unmount SD card: %s", esp_err_to_name(ret));
		return;
	}

	m_card = nullptr;
	m_mounted = false;
	Log::info(TAG, "SD card unmounted");
#endif
}

} // namespace flx::services
