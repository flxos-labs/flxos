#include <Config.hpp>
#include <flx/system/device/DeviceProfiles.hpp>

namespace flx::system {

static const DeviceProfile active_profile = {
#if defined(FLXOS_PROFILE_CYD_2432S028R)
	.profileId = "cyd-2432s028r",
	.vendor = "Sunton",
	.boardName = "CYD-2432S028R",
#elif defined(FLXOS_PROFILE_ESP32S3_ILI9341_XPT)
	.profileId = "esp32s3-ili9341-xpt",
	.vendor = "Espressif",
	.boardName = "ESP32-S3-ILI9341",
#else
	.profileId = "custom",
	.vendor = "Custom",
	.boardName = "Custom Board",
#endif
	.chipTarget = CONFIG_IDF_TARGET,
	.description = "Compile-time configured profile",

	.display = {
		.width = FLXOS_DISPLAY_WIDTH,
		.height = FLXOS_DISPLAY_HEIGHT,
		.driver = FLXOS_DISPLAY_DRIVER,
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
		.sizeInches = 0.0f // Not in Config.hpp usually
	},

	.touch = {
#if defined(FLXOS_TOUCH_ENABLED)
		.enabled = true,
		.driver = FLXOS_TOUCH_DRIVER,
#if defined(FLXOS_TOUCH_BUS_SHARED)
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

	.connectivity = {.wifi = true, // Espressif chips usually have WiFi
					 .bluetooth = true, // And BT
					 .bleOnly = false,
					 .flashSizeKb = 0, // Could read from sdkconfig
					 .psramSizeKb = 0}
};

const DeviceProfile& DeviceProfiles::get() {
	return active_profile;
}

} // namespace flx::system
