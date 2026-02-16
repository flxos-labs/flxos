#pragma once

#include <cstdint>
#include <flx/core/Singleton.hpp>
#include <flx/services/IService.hpp>
#include <flx/services/ServiceManifest.hpp>
#include <flx/system/device/DeviceProfile.hpp>
#include <string>
#include <vector>

namespace flx::services {

/**
 * @brief Service for device profile management.
 *
 * Provides:
 * - NVS pin overrides (no recompile for pin changes)
 * - Touch calibration persistence
 * - I2C bus scanning for peripheral auto-detection
 *
 * Boot flow:
 *   1. Reads CONFIG_FLXOS_DEVICE_PROFILE_ID from Kconfig
 *   2. Looks up matching DeviceProfile from the built-in registry
 *   3. Loads NVS pin overrides
 *   4. Loads saved touch calibration from NVS
 */
class DeviceProfileService : public IService, public flx::Singleton<DeviceProfileService> {
	friend class flx::Singleton<DeviceProfileService>;

public:

	// ──── IService ────

	static const ServiceManifest serviceManifest;
	const ServiceManifest& getManifest() const override { return serviceManifest; }

	bool onStart() override;
	void onStop() override;

	// ──── Profile Queries ────

	/** Get the active device profile */
	const flx::system::DeviceProfile& getActiveProfile() const;

	/** Get the active profile ID */
	const std::string& getActiveProfileId() const;

	/** Check if a valid profile is loaded */
	bool hasValidProfile() const { return m_activeProfile != nullptr; }

	// ──── NVS Pin Overrides ────

	/**
	 * Get a pin number, checking NVS overrides first, then falling back to default.
	 * @param pinName  Logical pin name (e.g. "sd_cs", "touch_int")
	 * @param kconfigDefault  Compile-time default from Kconfig
	 * @return Overridden pin if set in NVS, otherwise kconfigDefault
	 */
	int getPin(const std::string& pinName, int kconfigDefault) const;

	/**
	 * Save a pin override to NVS. Takes effect on next boot.
	 */
	bool setPinOverride(const std::string& pinName, int gpioNum);

	/**
	 * Remove a pin override from NVS, reverting to Kconfig default.
	 */
	bool clearPinOverride(const std::string& pinName);

	/**
	 * Check if a pin has an NVS override.
	 */
	bool hasPinOverride(const std::string& pinName) const;

	// ──── Touch Calibration Persistence ────

	/**
	 * Save touch calibration data to NVS.
	 */
	bool saveTouchCalibration(int16_t xMin, int16_t xMax, int16_t yMin, int16_t yMax);

	/**
	 * Load touch calibration data from NVS.
	 * @return true if valid calibration was loaded
	 */
	bool loadTouchCalibration(int16_t& xMin, int16_t& xMax, int16_t& yMin, int16_t& yMax) const;

	/**
	 * Check if saved touch calibration exists.
	 */
	bool hasTouchCalibration() const;

	// ──── I2C Auto-Detection ────

	struct I2CDetectResult {
		uint8_t address;
		std::string suggestedDriver; ///< e.g. "GT911", "FT5x06", "SSD1306"
	};

	/**
	 * Scan I2C bus for connected peripherals and identify them.
	 * @param sdaPin  SDA GPIO pin
	 * @param sclPin  SCL GPIO pin
	 * @param port    I2C port number (0 or 1)
	 * @return List of detected devices with suggested driver names
	 */
	std::vector<I2CDetectResult> scanI2CBus(int sdaPin, int sclPin, int port = 0) const;

private:

	DeviceProfileService() = default;
	~DeviceProfileService() = default;

	const flx::system::DeviceProfile* m_activeProfile = nullptr;
	flx::system::DeviceProfile m_fallbackProfile; ///< Used if no matching profile found

	static const std::string NVS_NAMESPACE_PINS;
	static const std::string NVS_NAMESPACE_TOUCH;
};

} // namespace flx::services
