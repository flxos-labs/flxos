#pragma once

#include <cstdint>
#include <string>

namespace System {

/**
 * @brief Describes the hardware configuration of a specific board/device.
 *
 * Rich runtime-queryable metadata including connectivity flags,
 * PSRAM size, feature flags, and display DPI.
 */
struct DeviceProfile {

	// ──── Identity ────

	std::string profileId; ///< Unique ID, e.g. "cyd-2432s028r"
	std::string vendor; ///< e.g. "CYD", "LilyGo", "Espressif"
	std::string boardName; ///< e.g. "2432S028R"
	std::string chipTarget; ///< e.g. "ESP32", "ESP32-S3", "ESP32-C3"
	std::string description; ///< Human-readable board description

	// ──── Display ────

	struct DisplayConfig {
		uint16_t width = 240;
		uint16_t height = 320;
		uint16_t rotation = 90; ///< Default rotation in degrees
		std::string driver; ///< e.g. "ILI9341", "ST7789", "SSD1306"
		std::string bus; ///< e.g. "SPI", "I2C", "Parallel8", "Parallel16"
		float sizeInches = 0; ///< Physical display size
		uint16_t dpi = 0; ///< Dots per inch
		bool invertColors = false;
		bool rgbOrder = false; ///< true=RGB, false=BGR
	} display;

	// ──── Touch ────

	struct TouchConfig {
		bool enabled = false;
		std::string driver; ///< e.g. "XPT2046", "GT911", "FT5x06"
		std::string bus; ///< "SPI" or "I2C"
		bool separateBus = false; ///< Touch on separate SPI bus from display
	} touch;

	// ──── SD Card ────

	struct SdCardConfig {
		bool supported = false;
		std::string mode; ///< "SPI" or "SDMMC"
	} sdCard;

	// ──── Battery ────

	struct BatteryConfig {
		bool supported = false;
		uint16_t maxVoltageMv = 4200;
		uint16_t minVoltageMv = 3300;
	} battery;

	// ──── Connectivity ────

	struct ConnectivityConfig {
		bool wifi = true;
		bool bluetooth = true;
		bool bleOnly = false; ///< true for ESP32-C3 (BLE only, no classic BT)
		uint32_t flashSizeKb = 4096;
		uint32_t psramSizeKb = 0;
	} connectivity;

	// ──── Extra Features ────

	struct Features {
		bool hasRgbLed = false;
		bool hasBuiltinSpeaker = false;
		bool hasExternalAntenna = false;
	} features;

	// ──── Capability Queries ────

	bool hasWifi() const { return connectivity.wifi; }
	bool hasBluetooth() const { return connectivity.bluetooth; }
	bool hasPsram() const { return connectivity.psramSizeKb > 0; }
	bool hasTouch() const { return touch.enabled; }
	bool hasSdCard() const { return sdCard.supported; }
	bool hasBattery() const { return battery.supported; }
};

} // namespace System
