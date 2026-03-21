#include "Config.hpp"
#include <flx/core/GuiLockGuard.hpp>
#include <flx/core/Logger.hpp>
#include <flx/hal/sdcard/SdmmcSdCardDevice.hpp>

#if FLXOS_SD_CARD_ENABLED
#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#endif

namespace flx::hal::sdcard {

static constexpr std::string_view TAG = "SdmmcSdCardDevice";

SdmmcSdCardDevice::SdmmcSdCardDevice() {
	this->setState(State::Uninitialized);
}

SdmmcSdCardDevice::~SdmmcSdCardDevice() {
	if (getState() == State::Ready || m_mountState == MountState::Mounted) {
		stop();
	}
}

bool SdmmcSdCardDevice::start() {
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

bool SdmmcSdCardDevice::stop() {
#if FLXOS_SD_CARD_ENABLED
	if (m_mountState == MountState::Mounted) {
		unmount();
	}
#endif
	this->setState(State::Stopped);
	return true;
}

bool SdmmcSdCardDevice::mount(const std::string& mountPath) {
#if FLXOS_SD_CARD_ENABLED
	flx::core::GuiLockGuard lock;
	if (m_mountState == MountState::Mounted) {
		return true;
	}

	m_mountPath = mountPath;
	flx::Log::info(TAG, "Mounting SD card via SDMMC at %s...", m_mountPath.c_str());

	const esp_vfs_fat_sdmmc_mount_config_t mount_config = {
		.format_if_mount_failed = false,
		.max_files = 5,
		.allocation_unit_size = 16 * 1024,
		.disk_status_check_enable = false,
		.use_one_fat = false,
	};

	sdmmc_host_t host = SDMMC_HOST_DEFAULT();
	host.max_freq_khz = flx::config::sdcard.maxFreqKhz;

	sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

	// If pins are specified in config, we should set them.
	// Note: On some ESP32 targets, SDMMC pins are fixed. On S3 they can be remapped.
	if (flx::config::sdcard.sdmmcPins.clk != -1) {
		// Basic validation: clk, cmd, and d0 must all be set if any remapping is done
		if (flx::config::sdcard.sdmmcPins.cmd == -1 || flx::config::sdcard.sdmmcPins.d0 == -1) {
			flx::Log::error(TAG, "Incomplete SDMMC pin configuration (requires clk, cmd, and d0).");
			m_mountState = MountState::Error;
			return false;
		}

		slot_config.clk = (gpio_num_t)flx::config::sdcard.sdmmcPins.clk;
		slot_config.cmd = (gpio_num_t)flx::config::sdcard.sdmmcPins.cmd;
		slot_config.d0 = (gpio_num_t)flx::config::sdcard.sdmmcPins.d0;

		const int cfg_d1 = flx::config::sdcard.sdmmcPins.d1;
		const int cfg_d2 = flx::config::sdcard.sdmmcPins.d2;
		const int cfg_d3 = flx::config::sdcard.sdmmcPins.d3;

		// 4-bit mode: d1, d2, and d3 must all be set together.
		// 1-bit mode with explicit D3: only d3 is set (d1 and d2 remain -1).
		//   This is needed on boards where the SD card CS/D3 pin must be managed
		//   by the SDMMC driver to properly take the card out of SPI mode.
		const bool is_4bit = (cfg_d1 != -1 && cfg_d2 != -1 && cfg_d3 != -1);
		const bool d3_only = (cfg_d1 == -1 && cfg_d2 == -1 && cfg_d3 != -1);
		const bool any_d123 = (cfg_d1 != -1 || cfg_d2 != -1 || cfg_d3 != -1);

		if (any_d123 && !is_4bit && !d3_only) {
			flx::Log::error(TAG, "Incomplete SDMMC pin configuration (requires d1, d2, and d3 for 4-bit mode).");
			m_mountState = MountState::Error;
			return false;
		}

		if (is_4bit) {
			slot_config.d1 = (gpio_num_t)cfg_d1;
			slot_config.d2 = (gpio_num_t)cfg_d2;
			slot_config.d3 = (gpio_num_t)cfg_d3;
			slot_config.width = 4;
		} else {
			// 1-bit mode: configure D3 if explicitly specified (needed as CS on some boards)
			if (d3_only) {
				slot_config.d3 = (gpio_num_t)cfg_d3;
			}
			slot_config.width = 1;
		}
	}

	sdmmc_card_t* card_local = nullptr;
	const esp_err_t ret = esp_vfs_fat_sdmmc_mount(
		m_mountPath.c_str(), &host, &slot_config, &mount_config, &card_local);

	if (ret != ESP_OK) {
		flx::Log::error(TAG, "Failed to mount SD card via SDMMC: %s", esp_err_to_name(ret));
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

bool SdmmcSdCardDevice::unmount() {
#if FLXOS_SD_CARD_ENABLED
	flx::core::GuiLockGuard lock;
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

ISdCardDevice::MountState SdmmcSdCardDevice::getMountState() const {
	return m_mountState;
}

std::string SdmmcSdCardDevice::getMountPath() const {
	return m_mountPath;
}

std::recursive_mutex& SdmmcSdCardDevice::getLock() {
	return m_lock;
}

bool SdmmcSdCardDevice::getCardInfo(CardInfo& info) const {
#if FLXOS_SD_CARD_ENABLED
	flx::core::GuiLockGuard lock;
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
