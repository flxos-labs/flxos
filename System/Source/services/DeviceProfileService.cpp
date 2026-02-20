#include "Config.hpp"
#include <flx/core/Logger.hpp>
#include <flx/system/device/DeviceProfile.hpp>
#include <flx/system/services/DeviceProfileService.hpp>

#include "driver/i2c_master.h"
#include "nvs.h"
#include "nvs_flash.h"

#include <cstring>

static constexpr const char* TAG = "DeviceProfile";

namespace flx::services {

// NVS namespace names (max 15 chars)
const std::string DeviceProfileService::NVS_NAMESPACE_PINS = "flxos_pins";
const std::string DeviceProfileService::NVS_NAMESPACE_TOUCH = "flxos_touch";

// ============================================================================
// Service Manifest
// ============================================================================

const ServiceManifest DeviceProfileService::serviceManifest = {
	.serviceId = "com.flxos.device_profile",
	.serviceName = "Device Profile",
	.version = "1.0.0",
	.dependencies = {}, // No dependencies — starts very early
	.priority = 5, // Very high priority (before display)
	.required = false, // System can still boot without a valid profile
	.autoStart = true,
	.guiRequired = false,
	.capabilities = ServiceCapability::None,
	.description = "Device hardware profile management with NVS overrides",
};

// ============================================================================
// Active Profile Definition
// ============================================================================

static const flx::system::DeviceProfile ACTIVE_PROFILE = {
#if defined(FLXOS_PROFILE_ID)
	.profileId = FLXOS_PROFILE_ID,
	.vendor = FLXOS_PROFILE_VENDOR,
	.boardName = FLXOS_PROFILE_BOARD_NAME,
#else
	.profileId = "unknown",
	.vendor = "Unknown",
	.boardName = "Unknown Board",
#endif
	.chipTarget = CONFIG_IDF_TARGET,
	.description = "Compile-time configured profile",

	.display = {
#if defined(FLXOS_DISPLAY_WIDTH)
		.width = FLXOS_DISPLAY_WIDTH,
		.height = FLXOS_DISPLAY_HEIGHT,
		.driver = FLXOS_DISPLAY_DRIVER,
#else
		.width = 0,
		.height = 0,
		.driver = "None",
#endif

#if defined(FLXOS_BUS_SPI)
		.bus = "SPI",
#elif defined(FLXOS_BUS_I2C)
		.bus = "I2C",
#elif defined(FLXOS_BUS_PARALLEL8)
		.bus = "Parallel 8-bit",
#elif defined(FLXOS_BUS_PARALLEL16)
		.bus = "Parallel 16-bit",
#else
		.bus = "Unknown",
#endif

#if defined(FLXOS_DISPLAY_SIZE_INCHES)
		.sizeInches = FLXOS_DISPLAY_SIZE_INCHES
#else
		.sizeInches = 0.0f
#endif
	},

	.touch = {
#if defined(FLXOS_TOUCH_ENABLED) && FLXOS_TOUCH_ENABLED
		.enabled = true,
		.driver = FLXOS_TOUCH_DRIVER,
#if defined(FLXOS_TOUCH_BUS_SHARED) && FLXOS_TOUCH_BUS_SHARED
		.bus = "Shared",
		.separateBus = false,
#else
		.bus = "Separate",
		.separateBus = true,
#endif
#else
		.enabled = false,
		.driver = "None",
		.bus = "None",
		.separateBus = false
#endif
	},

	.sdCard = {
#if defined(FLXOS_SD_CS) || defined(FLXOS_SD_PIN_CLK)
		.supported = true,
		.mode = "SPI" // Simplification
#else
		.supported = false,
		.mode = "None"
#endif
	},

	.connectivity = {
		.wifi = true, // Espressif chips usually have WiFi
		.bluetooth = true, // And BT
		.bleOnly = false,
		.flashSizeKb = 0, // Could read from sdkconfig, but unused in UI
#if defined(CONFIG_ESP32S3_SPIRAM_SUPPORT) || defined(CONFIG_SPIRAM) || defined(CONFIG_ESP32_SPIRAM_SUPPORT)
		.psramSizeKb = 2048 // Dummy non-zero to show feature
#else
		.psramSizeKb = 0
#endif
	}
};

// ============================================================================
// IService Lifecycle
// ============================================================================

bool DeviceProfileService::onStart() {
	Log::info(TAG, "Loading device profile...");

	// 1. Get the single active profile (compile-time)
	const flx::system::DeviceProfile& profile = ACTIVE_PROFILE;
	m_activeProfile = &profile;

	Log::info(TAG, "Profile loaded: %s %s (%s)", profile.vendor.c_str(), profile.boardName.c_str(), profile.chipTarget.c_str());
	Log::info(TAG, "  Display: %ux%u %s via %s (%.1f\")", profile.display.width, profile.display.height, profile.display.driver.c_str(), profile.display.bus.c_str(), profile.display.sizeInches);

	if (profile.touch.enabled) {
		Log::info(TAG, "  Touch: %s via %s%s", profile.touch.driver.c_str(), profile.touch.bus.c_str(), profile.touch.separateBus ? " (separate bus)" : "");
	}

	if (profile.sdCard.supported) {
		Log::info(TAG, "  SD Card: %s", profile.sdCard.mode.c_str());
	}

	Log::info(TAG, "  Connectivity: WiFi=%s, BT=%s%s, Flash=%luKB, PSRAM=%luKB", profile.connectivity.wifi ? "yes" : "no", profile.connectivity.bluetooth ? "yes" : "no", profile.connectivity.bleOnly ? " (BLE only)" : "", (unsigned long)profile.connectivity.flashSizeKb, (unsigned long)profile.connectivity.psramSizeKb);

	// 2. Check for touch calibration data
	if (hasTouchCalibration()) {
		Log::info(TAG, "Saved touch calibration found in NVS");
	}

	Log::info(TAG, "Device profile service started");
	return true;
}

void DeviceProfileService::onStop() {
	m_activeProfile = nullptr;
	Log::info(TAG, "Device profile service stopped");
}

// ============================================================================
// Profile Queries
// ============================================================================

const flx::system::DeviceProfile& DeviceProfileService::getActiveProfile() const {
	if (m_activeProfile) {
		return *m_activeProfile;
	}
	// Should never happen after onStart()
	static const flx::system::DeviceProfile empty {};
	return empty;
}

const std::string& DeviceProfileService::getActiveProfileId() const {
	return getActiveProfile().profileId;
}

// ============================================================================
// NVS Pin Overrides
// ============================================================================

int DeviceProfileService::getPin(const std::string& pinName, int kconfigDefault) const {
	nvs_handle_t handle;
	esp_err_t err = nvs_open(NVS_NAMESPACE_PINS.c_str(), NVS_READONLY, &handle);
	if (err != ESP_OK) {
		return kconfigDefault;
	}

	int32_t value = kconfigDefault;
	err = nvs_get_i32(handle, pinName.c_str(), &value);
	nvs_close(handle);

	if (err == ESP_OK) {
		Log::debug(TAG, "Pin '%s' overridden: %d → %ld", pinName.c_str(), kconfigDefault, (long)value);
		return static_cast<int>(value);
	}
	return kconfigDefault;
}

bool DeviceProfileService::setPinOverride(const std::string& pinName, int gpioNum) {
	nvs_handle_t handle;
	esp_err_t err = nvs_open(NVS_NAMESPACE_PINS.c_str(), NVS_READWRITE, &handle);
	if (err != ESP_OK) {
		Log::error(TAG, "Failed to open NVS for pin override: %s", esp_err_to_name(err));
		return false;
	}

	err = nvs_set_i32(handle, pinName.c_str(), static_cast<int32_t>(gpioNum));
	if (err == ESP_OK) {
		err = nvs_commit(handle);
		if (err == ESP_OK) {
			Log::info(TAG, "Pin override saved: %s = GPIO %d (takes effect on next boot)", pinName.c_str(), gpioNum);
		} else {
			Log::error(TAG, "Failed to commit pin override: %s", esp_err_to_name(err));
		}
	} else {
		Log::error(TAG, "Failed to save pin override: %s", esp_err_to_name(err));
	}

	nvs_close(handle);
	return err == ESP_OK;
}

bool DeviceProfileService::clearPinOverride(const std::string& pinName) {
	nvs_handle_t handle;
	esp_err_t err = nvs_open(NVS_NAMESPACE_PINS.c_str(), NVS_READWRITE, &handle);
	if (err != ESP_OK) {
		return false;
	}

	err = nvs_erase_key(handle, pinName.c_str());
	if (err == ESP_OK) {
		err = nvs_commit(handle);
		if (err == ESP_OK) {
			Log::info(TAG, "Pin override cleared: %s", pinName.c_str());
		} else {
			Log::error(TAG, "Failed to commit pin override clear: %s", esp_err_to_name(err));
		}
	}

	nvs_close(handle);
	return err == ESP_OK;
}

bool DeviceProfileService::hasPinOverride(const std::string& pinName) const {
	nvs_handle_t handle;
	esp_err_t err = nvs_open(NVS_NAMESPACE_PINS.c_str(), NVS_READONLY, &handle);
	if (err != ESP_OK) {
		return false;
	}

	int32_t value;
	err = nvs_get_i32(handle, pinName.c_str(), &value);
	nvs_close(handle);
	return err == ESP_OK;
}

// ============================================================================
// Touch Calibration Persistence
// ============================================================================

bool DeviceProfileService::saveTouchCalibration(int16_t xMin, int16_t xMax, int16_t yMin, int16_t yMax) {
	nvs_handle_t handle;
	esp_err_t err = nvs_open(NVS_NAMESPACE_TOUCH.c_str(), NVS_READWRITE, &handle);
	if (err != ESP_OK) {
		Log::error(TAG, "Failed to open NVS for touch calibration: %s", esp_err_to_name(err));
		return false;
	}

	struct {
		int16_t xMin, xMax, yMin, yMax;
	} cal = {xMin, xMax, yMin, yMax};

	err = nvs_set_blob(handle, "cal", &cal, sizeof(cal));
	if (err == ESP_OK) {
		err = nvs_commit(handle);
		if (err == ESP_OK) {
			Log::info(TAG, "Touch calibration saved: xMin=%d xMax=%d yMin=%d yMax=%d", xMin, xMax, yMin, yMax);
		} else {
			Log::error(TAG, "Failed to commit touch calibration: %s", esp_err_to_name(err));
		}
	} else {
		Log::error(TAG, "Failed to save touch calibration: %s", esp_err_to_name(err));
	}

	nvs_close(handle);
	return err == ESP_OK;
}

bool DeviceProfileService::loadTouchCalibration(int16_t& xMin, int16_t& xMax, int16_t& yMin, int16_t& yMax) const {
	nvs_handle_t handle;
	esp_err_t err = nvs_open(NVS_NAMESPACE_TOUCH.c_str(), NVS_READONLY, &handle);
	if (err != ESP_OK) {
		return false;
	}

	struct {
		int16_t xMin, xMax, yMin, yMax;
	} cal {};

	size_t len = sizeof(cal);
	err = nvs_get_blob(handle, "cal", &cal, &len);
	nvs_close(handle);

	if (err == ESP_OK && len == sizeof(cal)) {
		xMin = cal.xMin;
		xMax = cal.xMax;
		yMin = cal.yMin;
		yMax = cal.yMax;

		// Sanity check
		if (xMin == xMax || yMin == yMax) {
			Log::warn(TAG, "Stored touch calibration invalid (degenerate)");
			return false;
		}
		return true;
	}
	return false;
}

bool DeviceProfileService::hasTouchCalibration() const {
	int16_t x0, x1, y0, y1;
	return loadTouchCalibration(x0, x1, y0, y1);
}

// ============================================================================
// I2C Auto-Detection
// ============================================================================

// Known I2C device addresses → driver suggestions
struct KnownI2CDevice {
	uint8_t address;
	const char* name;
};

static const KnownI2CDevice KNOWN_I2C_DEVICES[] = {
	{0x14, "GT911"}, // Goodix touch controller
	{0x15, "FT5x06"}, // FocalTech capacitive touch
	{0x38, "FT5x06"}, // FocalTech secondary address
	{0x3C, "SSD1306"}, // OLED display
	{0x3D, "SSD1306"}, // OLED display (alt)
	{0x48, "ADS1115"}, // ADC
	{0x50, "EEPROM"}, // 24C02+ EEPROM
	{0x51, "PCF8563"}, // RTC
	{0x57, "MAX30102"}, // Heart rate / SpO2
	{0x5A, "CSTxxx"}, // Hynitron capacitive touch
	{0x5D, "GT911"}, // Goodix secondary address
	{0x68, "MPU6050"}, // IMU
	{0x76, "BME280"}, // Temp/Humidity/Pressure
	{0x77, "BMP280"}, // Temp/Pressure
};

std::vector<DeviceProfileService::I2CDetectResult>
DeviceProfileService::scanI2CBus(int sdaPin, int sclPin, int port) const {
	std::vector<I2CDetectResult> results;

	// Configure I2C master bus
	i2c_master_bus_config_t bus_config = {};
	bus_config.clk_source = I2C_CLK_SRC_DEFAULT;
	bus_config.i2c_port = port;
	bus_config.scl_io_num = static_cast<gpio_num_t>(sclPin);
	bus_config.sda_io_num = static_cast<gpio_num_t>(sdaPin);
	bus_config.glitch_ignore_cnt = 7;
	bus_config.flags.enable_internal_pullup = true;

	i2c_master_bus_handle_t bus_handle = nullptr;
	esp_err_t err = i2c_new_master_bus(&bus_config, &bus_handle);
	if (err != ESP_OK) {
		Log::error(TAG, "Failed to init I2C bus for scan: %s", esp_err_to_name(err));
		return results;
	}

	Log::info(TAG, "Scanning I2C bus (port=%d, SDA=%d, SCL=%d)...", port, sdaPin, sclPin);

	for (uint8_t addr = 0x08; addr < 0x78; addr++) {
		err = i2c_master_probe(bus_handle, addr, 50); // 50ms timeout
		if (err == ESP_OK) {
			I2CDetectResult result;
			result.address = addr;
			result.suggestedDriver = "Unknown";

			// Match against known devices
			for (const auto& known: KNOWN_I2C_DEVICES) {
				if (known.address == addr) {
					result.suggestedDriver = known.name;
					break;
				}
			}

			Log::info(TAG, "  Found device at 0x%02X → %s", addr, result.suggestedDriver.c_str());
			results.push_back(result);
		}
	}

	i2c_del_master_bus(bus_handle);

	Log::info(TAG, "I2C scan complete: %zu device(s) found", results.size());
	return results;
}

} // namespace flx::services
