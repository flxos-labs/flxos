#pragma once

#include <flx/hal/DeviceBase.hpp>
#include <flx/hal/sdcard/ISdCardDevice.hpp>
#include <mutex>
#include <string>

namespace flx::hal::sdcard {

/**
 * @brief Concrete native SDMMC-backed SD Card implementation.
 *
 * Uses ESP-IDF's VFS FAT SDMMC APIs.
 * Typically used for boards with dedicated SDMMC pins (e.g. Lilygo T-HMI).
 */
class SdmmcSdCardDevice final : public flx::hal::DeviceBase<ISdCardDevice> {
public:

	SdmmcSdCardDevice() { setState(State::Uninitialized); }
	~SdmmcSdCardDevice() override {
		if (getState() == State::Ready) stop();
	}

	// ── IDevice ───────────────────────────────────────────────────────────
	std::string_view getName() const override { return "SDMMC SD Card"; }
	std::string_view getDescription() const override { return "Native SDMMC-attached SD card driver"; }

	// TODO: Implement native SDMMC start logic if necessary.
	bool start() override {
		// Device is an unimplemented stub. Return false so it is not registered.
		return false;
	}
	bool stop() override {
		return true;
	}

	// ── ISdCardDevice ─────────────────────────────────────────────────────
	// TODO: Implement actual Mount/Unmount logic for SDMMC via esp_vfs_fat_sdmmc_mount
	bool mount(const std::string& mountPath) override { return false; }
	bool unmount() override { return false; }
	MountState getMountState() const override { return MountState::Unmounted; }
	std::string getMountPath() const override { return ""; }
	std::recursive_mutex& getLock() override { return m_lock; }
	bool getCardInfo(CardInfo& info) const override { return false; }

private:

	std::recursive_mutex m_lock;
};

} // namespace flx::hal::sdcard
