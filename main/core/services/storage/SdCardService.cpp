#include "SdCardService.hpp"
#include <flx/core/Logger.hpp>
#include "sdkconfig.h"
#include <string_view>

#if defined(CONFIG_FLXOS_SD_CARD_ENABLED)
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
#if defined(CONFIG_FLXOS_SD_CARD_ENABLED)
	: m_mountPoint(CONFIG_FLXOS_SD_MOUNT_POINT)
#else
	: m_mountPoint("/sdcard")
#endif
{
}

SdCardService& SdCardService::getInstance() {
	static SdCardService instance;
	return instance;
}

bool SdCardService::onStart() {
	return mount();
}

void SdCardService::onStop() {
	unmount();
}

bool SdCardService::mount() {
#if !defined(CONFIG_FLXOS_SD_CARD_ENABLED)
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

	// SPI host config
	spi_host_device_t host_id = (CONFIG_FLXOS_SD_SPI_HOST == 1)
		? SPI2_HOST
		: SPI3_HOST;

	sdmmc_host_t host = SDSPI_HOST_DEFAULT();
	host.slot = host_id;
	host.max_freq_khz = CONFIG_FLXOS_SD_MAX_FREQ_KHZ;

	// SPI bus config
	int pin_mosi = CONFIG_FLXOS_SD_PIN_MOSI;
	int pin_miso = CONFIG_FLXOS_SD_PIN_MISO;
	int pin_sclk = CONFIG_FLXOS_SD_PIN_SCLK;

#if defined(CONFIG_FLXOS_BUS_SPI)
	if (pin_mosi == -1) pin_mosi = CONFIG_FLXOS_PIN_MOSI;
	if (pin_miso == -1) pin_miso = CONFIG_FLXOS_PIN_MISO;
	if (pin_sclk == -1) pin_sclk = CONFIG_FLXOS_PIN_SCLK;
#endif

	m_busInitializedHere = false;

	spi_bus_config_t bus_cfg = {};
	bus_cfg.mosi_io_num = pin_mosi;
	bus_cfg.miso_io_num = pin_miso;
	bus_cfg.sclk_io_num = pin_sclk;
	bus_cfg.quadwp_io_num = -1;
	bus_cfg.quadhd_io_num = -1;
	bus_cfg.max_transfer_sz = 4000;

	esp_err_t ret = spi_bus_initialize(host_id, &bus_cfg, SDSPI_DEFAULT_DMA);
	if (ret == ESP_OK) {
		m_busInitializedHere = true;
		Log::info(TAG, "SPI bus initialized for SD card");
	} else if (ret == ESP_ERR_INVALID_STATE) {
		Log::info(TAG, "SPI bus already initialized (shared), attaching SD device");
	} else {
		Log::error(TAG, "Failed to initialize SPI bus: %s", esp_err_to_name(ret));
		return false;
	}

	sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
	slot_config.gpio_cs = (gpio_num_t)CONFIG_FLXOS_SD_PIN_CS;
	slot_config.host_id = host_id;

	ret = esp_vfs_fat_sdspi_mount(
		m_mountPoint.c_str(), &host, &slot_config, &mount_config, &m_card
	);

	if (ret != ESP_OK) {
		Log::error(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
		if (m_busInitializedHere) {
			spi_bus_free(host_id);
		}
		return false;
	}

	m_mounted = true;
	Log::info(TAG, "SD card mounted at %s", m_mountPoint.c_str());
	sdmmc_card_print_info(stdout, m_card);
	return true;
#endif
}

void SdCardService::unmount() {
#if defined(CONFIG_FLXOS_SD_CARD_ENABLED)
	if (!m_mounted) {
		return;
	}

	esp_err_t ret = esp_vfs_fat_sdcard_unmount(m_mountPoint.c_str(), m_card);
	if (ret != ESP_OK) {
		Log::error(TAG, "Failed to unmount SD card: %s", esp_err_to_name(ret));
		return;
	}

	if (m_busInitializedHere) {
		spi_host_device_t host_id = (CONFIG_FLXOS_SD_SPI_HOST == 1)
			? SPI2_HOST
			: SPI3_HOST;
		spi_bus_free(host_id);
	}

	m_card = nullptr;
	m_busInitializedHere = false;
	m_mounted = false;
	Log::info(TAG, "SD card unmounted");
#endif
}

} // namespace flx::services
