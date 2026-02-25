#pragma once

#include <cstdint>
#include <flx/hal/IDevice.hpp>

namespace flx::hal::usb {

/**
 * @brief Abstract interface for USB peripheral device controllers.
 *
 * Surpasses Tactility's simple tt::hal::usb namespace functions.
 * Encapsulates TinyUSB or native ESP-IDF USB driver.
 *
 * Supports switching modes dynamically (e.g. Mass Storage vs CDC Serial).
 */
class IUsbDevice : public flx::hal::IDevice {
public:

	// ── IDevice ───────────────────────────────────────────────────────────
	Type getType() const override { return Type::Usb; }

	enum class UsbMode : uint8_t {
		None, ///< USB disabled
		Default, ///< Default profile setting
		MassStorageSdCard, ///< Expose SD Card as flash drive
		MassStorageFlash, ///< Expose internal FAT/LittleFS as flash drive
		CdcSerial ///< USB CDC serial communication
	};

	virtual bool isSupported() const = 0;
	virtual UsbMode getCurrentMode() const = 0;

	// ── Mode switching ────────────────────────────────────────────────────
	/** Start USB in Mass Storage Class mode (MSC). */
	virtual bool startMassStorage(UsbMode mode) = 0;

	/** Start USB in Communications Device Class mode (CDC). */
	virtual bool startCdcSerial() { return false; }

	/** Stop USB completely. */
	virtual void stopMode() = 0;

	// ── Reboot handling ───────────────────────────────────────────────────
	/** Check if changing to this mode requires a chip reboot on this hardware. */
	virtual bool canReboot(UsbMode mode) const = 0;

	/** Restart system and enter this mode at next boot. */
	virtual void rebootInto(UsbMode mode) = 0;
};

} // namespace flx::hal::usb
