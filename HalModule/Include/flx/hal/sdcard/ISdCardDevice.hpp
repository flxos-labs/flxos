#pragma once

#include <cstdint>
#include <flx/hal/IDevice.hpp>
#include <mutex>
#include <string>

namespace flx::hal::sdcard {

/**
 * @brief Abstract interface for SD card / storage card devices.
 *
 * Implemented by:
 *  - SpiSdCardDevice (SPI-attached SD cards — most common)
 *  - SdmmcSdCardDevice (native SDMMC interface — T-HMI, etc.)
 *
 * Key improvements over Tactility's SdCardDevice:
 *  - MountState enum for fine-grained status
 *  - CardInfo struct (total/free bytes, FS type, max frequency)
 *  - Centralized bus lock for SPI contention management
 *  - isMounted() convenience over checking state directly
 */
class ISdCardDevice : public flx::hal::IDevice {
public:

	// ── IDevice ───────────────────────────────────────────────────────────
	Type getType() const override { return Type::SdCard; }

	// ── Mount state ───────────────────────────────────────────────────────
	enum class MountState : uint8_t {
		Unmounted,
		Mounted,
		Error,
		Unknown
	};

	/**
     * @brief Mount the SD card's filesystem at the given VFS path.
     * @param mountPath  VFS mount point, e.g. "/sdcard"
     * @return true if mounted successfully.
     */
	virtual bool mount(const std::string& mountPath) = 0;

	/**
     * @brief Unmount the filesystem cleanly.
     */
	virtual bool unmount() = 0;

	virtual MountState getMountState() const = 0;
	virtual std::string getMountPath() const = 0;
	virtual bool isMounted() const { return getMountState() == MountState::Mounted; }

	// ── Bus lock (surpasses Tactility) ────────────────────────────────────
	/**
     * @brief Direct access to the SPI bus mutex for this SD card.
     * Used by BusManager to prevent SPI contention with the display.
     * Tactility has no centralized bus contention management.
     */
	virtual std::recursive_mutex& getLock() = 0;

	// ── Card info (surpasses Tactility) ───────────────────────────────────
	/**
     * @brief Filesystem and capacity info from the mounted card.
     */
	struct CardInfo {
		uint64_t totalBytes = 0; ///< Total card capacity in bytes
		uint64_t freeBytes = 0; ///< Free space in bytes
		std::string fsType; ///< "FAT32", "exFAT", etc.
		uint32_t maxFreqKhz = 0; ///< Maximum clock frequency in kHz
	};

	/**
     * @brief Query filesystem capacity.
     * @param info  Output CardInfo struct.
     * @return true if card is mounted and info was read successfully.
     */
	virtual bool getCardInfo(CardInfo& info) const {
		(void)info;
		return false;
	}
};

} // namespace flx::hal::sdcard
