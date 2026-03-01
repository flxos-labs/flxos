#pragma once

#include <flx/hal/DeviceBase.hpp>
#include <flx/hal/sdcard/ISdCardDevice.hpp>
#include <mutex>
#include <string>

namespace flx::hal::sdcard {

/**
 * @brief Concrete SPI-backed SD Card implementation.
 *
 * Uses ESP-IDF's VFS FAT SPI SD Card APIs.
 * Supports centralized bus locking for SPI contention management.
 */
class SpiSdCardDevice final : public flx::hal::DeviceBase<ISdCardDevice> {
public:

	SpiSdCardDevice();
	~SpiSdCardDevice() override;

	// ── IDevice ───────────────────────────────────────────────────────────
	std::string_view getName() const override { return "SPI SD Card"; }
	std::string_view getDescription() const override { return "Default ESP-IDF SPI-attached SD card driver"; }
	bool start() override;
	bool stop() override;

	// ── ISdCardDevice ─────────────────────────────────────────────────────
	bool mount(const std::string& mountPath) override;
	bool unmount() override;
	MountState getMountState() const override;
	std::string getMountPath() const override;
	std::recursive_mutex& getLock() override;
	bool getCardInfo(CardInfo& info) const override;

private:

	std::string m_mountPath;
	MountState m_mountState {MountState::Unmounted};
	std::recursive_mutex m_spiLock;
	void* m_card {nullptr};
	bool m_spiOwner {false};

	void initSpiBus();
};

} // namespace flx::hal::sdcard
