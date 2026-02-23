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
	.dependencies = {},
	.priority = 5,
	.required = false,
	.autoStart = true,
	.guiRequired = false,
	.capabilities = ServiceCapability::None,
	.description = "Device hardware profile management with NVS overrides",
};

// ============================================================================
// Active Profile Definition
// ============================================================================

static const flx::system::DeviceProfile ACTIVE_PROFILE = {
	.profileId = flx::config::profile.id,
	.vendor = flx::config::profile.vendor,
	.boardName = flx::config::profile.boardName,
	.chipTarget = CONFIG_IDF_TARGET,
	.description = "Compile-time configured profile",

	.display = {
		.width = flx::config::display.enabled ? flx::config::display.width : 0,
		.height = flx::config::display.enabled ? flx::config::display.height : 0,
		.driver = flx::config::display.enabled ? flx::config::display.driver : "None",
		.bus = flx::config::display.enabled ? "SPI" : "None",
		.sizeInches = flx::config::display.sizeInches
	},

	.touch = {
		.enabled = flx::config::touch.enabled,
		.driver = flx::config::touch.enabled ? flx::config::touch.driver : "None",
		.bus = flx::config::touch.enabled ? (flx::config::touch.spi.busShared ? "Shared" : "Separate") : "None",
		.separateBus = flx::config::touch.enabled ? flx::config::touch.spi.separatePins : false,
	},

	.sdCard = {.supported = flx::config::sdcard.enabled, .mode = flx::config::sdcard.enabled ? "SPI" : "None"},

	.connectivity = {.wifi = flx::config::capabilities.wifi, .bluetooth = flx::config::capabilities.bluetooth, .bleOnly = false, .flashSizeKb = 0,
#if defined(CONFIG_ESP32S3_SPIRAM_SUPPORT) || defined(CONFIG_SPIRAM) || defined(CONFIG_ESP32_SPIRAM_SUPPORT)
					 .psramSizeKb = 2048
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

struct KnownI2CDevice {
	uint8_t address;
	const char* name;
};

static const KnownI2CDevice KNOWN_I2C_DEVICES[] = {
	{0x14, "GT911"},
	{0x15, "FT5x06"},
	{0x38, "FT5x06"},
	{0x3C, "SSD1306"},
	{0x3D, "SSD1306"},
	{0x48, "ADS1115"},
	{0x50, "EEPROM"},
	{0x51, "PCF8563"},
	{0x57, "MAX30102"},
	{0x5A, "CSTxxx"},
	{0x5D, "GT911"},
	{0x68, "MPU6050"},
	{0x76, "BME280"},
	{0x77, "BMP280"},
};

std::vector<DeviceProfileService::I2CDetectResult>
DeviceProfileService::scanI2CBus(int sdaPin, int sclPin, int port) const {
	std::vector<I2CDetectResult> results;

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
		err = i2c_master_probe(bus_handle, addr, 50);
		if (err == ESP_OK) {
			I2CDetectResult result;
			result.address = addr;
			result.suggestedDriver = "Unknown";

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
