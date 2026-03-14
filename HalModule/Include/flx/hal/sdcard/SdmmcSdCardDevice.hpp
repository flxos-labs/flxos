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

	SdmmcSdCardDevice();
	~SdmmcSdCardDevice() override;

	// ── IDevice ───────────────────────────────────────────────────────────
	std::string_view getName() const override { return "SDMMC SD Card"; }
	std::string_view getDescription() const override { return "Native SDMMC-attached SD card driver"; }

	bool start() override;
	bool stop() override;

	// ── ISdCardDevice ─────────────────────────────────────────────────────
	bool mount(const std::string& mountPath) override;
	bool unmount() override;
	ISdCardDevice::MountState getMountState() const override;
	std::string getMountPath() const override;
	std::recursive_mutex& getLock() override;
	bool getCardInfo(ISdCardDevice::CardInfo& info) const override;

private:

	std::string m_mountPath;
	ISdCardDevice::MountState m_mountState {ISdCardDevice::MountState::Unmounted};
	std::recursive_mutex m_lock;
	void* m_card {nullptr};
};

} // namespace flx::hal::sdcard
