#include "DeviceProfiles.hpp"
#include <cstddef>

namespace System {

// ============================================================================
// Built-in Device Profiles
// ============================================================================

static const DeviceProfile PROFILE_GENERIC_ESP32 = {
	.profileId = "generic-esp32",
	.vendor = "Espressif",
	.boardName = "Generic ESP32",
	.chipTarget = "ESP32",
	.description = "Generic ESP32 board with no specific hardware assumptions",
	.display = {
		.width = 240,
		.height = 320,
		.rotation = 90,
		.driver = "ILI9341",
		.bus = "SPI",
		.sizeInches = 2.8f,
		.dpi = 143,
		.invertColors = false,
		.rgbOrder = false,
	},
	.touch = {
		.enabled = true,
		.driver = "XPT2046",
		.bus = "SPI",
		.separateBus = false,
	},
	.sdCard = {
		.supported = false,
		.mode = "",
	},
	.battery = {
		.supported = false,
	},
	.connectivity = {
		.wifi = true,
		.bluetooth = true,
		.bleOnly = false,
		.flashSizeKb = 4096,
		.psramSizeKb = 0,
	},
	.features = {
		.hasRgbLed = false,
		.hasBuiltinSpeaker = false,
		.hasExternalAntenna = false,
	},
};

static const DeviceProfile PROFILE_GENERIC_ESP32S3 = {
	.profileId = "generic-esp32s3",
	.vendor = "Espressif",
	.boardName = "Generic ESP32-S3",
	.chipTarget = "ESP32-S3",
	.description = "Generic ESP32-S3 board with PSRAM and USB-OTG",
	.display = {
		.width = 240,
		.height = 320,
		.rotation = 90,
		.driver = "ST7789",
		.bus = "SPI",
		.sizeInches = 2.0f,
		.dpi = 200,
		.invertColors = false,
		.rgbOrder = false,
	},
	.touch = {
		.enabled = false,
		.driver = "",
		.bus = "",
		.separateBus = false,
	},
	.sdCard = {
		.supported = false,
		.mode = "",
	},
	.battery = {
		.supported = false,
	},
	.connectivity = {
		.wifi = true,
		.bluetooth = true,
		.bleOnly = false,
		.flashSizeKb = 16384, // 16MB flash common on S3
		.psramSizeKb = 8192, // 8MB PSRAM common
	},
	.features = {
		.hasRgbLed = true, // Most S3 devkits have addressable LED
		.hasBuiltinSpeaker = false,
		.hasExternalAntenna = false,
	},
};

static const DeviceProfile PROFILE_GENERIC_ESP32C3 = {
	.profileId = "generic-esp32c3",
	.vendor = "Espressif",
	.boardName = "Generic ESP32-C3",
	.chipTarget = "ESP32-C3",
	.description = "Generic ESP32-C3 board (RISC-V, BLE only, no classic Bluetooth)",
	.display = {
		.width = 128,
		.height = 64,
		.rotation = 0,
		.driver = "SSD1306",
		.bus = "I2C",
		.sizeInches = 0.96f,
		.dpi = 160,
		.invertColors = false,
		.rgbOrder = false,
	},
	.touch = {
		.enabled = false,
		.driver = "",
		.bus = "",
		.separateBus = false,
	},
	.sdCard = {
		.supported = false,
		.mode = "",
	},
	.battery = {
		.supported = false,
	},
	.connectivity = {
		.wifi = true,
		.bluetooth = true,
		.bleOnly = true, // C3 has BLE only
		.flashSizeKb = 4096,
		.psramSizeKb = 0,
	},
	.features = {
		.hasRgbLed = true,
		.hasBuiltinSpeaker = false,
		.hasExternalAntenna = false,
	},
};

static const DeviceProfile PROFILE_CYD_2432S028R = {
	.profileId = "cyd-2432s028r",
	.vendor = "CYD",
	.boardName = "2432S028R",
	.chipTarget = "ESP32",
	.description = "Cheap Yellow Display 2.8\" ILI9341 + XPT2046 touch (v1 original)",
	.display = {
		.width = 240,
		.height = 320,
		.rotation = 90,
		.driver = "ILI9341",
		.bus = "SPI",
		.sizeInches = 2.8f,
		.dpi = 143,
		.invertColors = false,
		.rgbOrder = false,
	},
	.touch = {
		.enabled = true,
		.driver = "XPT2046",
		.bus = "SPI",
		.separateBus = true, // CYD v1 has touch on separate SPI pins
	},
	.sdCard = {
		.supported = true,
		.mode = "SPI",
	},
	.battery = {
		.supported = false,
	},
	.connectivity = {
		.wifi = true,
		.bluetooth = true,
		.bleOnly = false,
		.flashSizeKb = 4096,
		.psramSizeKb = 0,
	},
	.features = {
		.hasRgbLed = true, // CYD v1 has an onboard RGB LED
		.hasBuiltinSpeaker = false,
		.hasExternalAntenna = false,
	},
};

// ============================================================================
// Profile Table
// ============================================================================

static const DeviceProfile* const ALL_PROFILES[] = {
	&PROFILE_GENERIC_ESP32,
	&PROFILE_GENERIC_ESP32S3,
	&PROFILE_GENERIC_ESP32C3,
	&PROFILE_CYD_2432S028R,
};

static constexpr size_t ALL_PROFILES_COUNT = sizeof(ALL_PROFILES) / sizeof(ALL_PROFILES[0]);

// ============================================================================
// Lookup Implementation
// ============================================================================

const DeviceProfile* DeviceProfiles::findById(const std::string& profileId) {
	for (size_t i = 0; i < ALL_PROFILES_COUNT; ++i) {
		if (ALL_PROFILES[i]->profileId == profileId) {
			return ALL_PROFILES[i];
		}
	}
	return nullptr;
}

std::vector<const DeviceProfile*> DeviceProfiles::getAll() {
	std::vector<const DeviceProfile*> result;
	result.reserve(ALL_PROFILES_COUNT);
	for (size_t i = 0; i < ALL_PROFILES_COUNT; ++i) {
		result.push_back(ALL_PROFILES[i]);
	}
	return result;
}

std::vector<std::string> DeviceProfiles::getAllIds() {
	std::vector<std::string> ids;
	ids.reserve(ALL_PROFILES_COUNT);
	for (size_t i = 0; i < ALL_PROFILES_COUNT; ++i) {
		ids.push_back(ALL_PROFILES[i]->profileId);
	}
	return ids;
}

size_t DeviceProfiles::count() {
	return ALL_PROFILES_COUNT;
}

} // namespace System
