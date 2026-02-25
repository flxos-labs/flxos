#pragma once

#include <cstdint>

// Forward declare the profile settings structure since hal shouldn't depend on system headers
namespace flx::config {
// These are auto-generated from profile.yaml
extern const bool hardware_display_enabled;
extern const bool hardware_touch_enabled;
extern const bool hardware_sdcard_enabled;
extern const bool hardware_power_enabled;
extern const bool hardware_gps_enabled;
extern const bool hardware_keyboard_enabled;
extern const bool hardware_encoder_enabled;
extern const bool hardware_usb_enabled;
extern const int hardware_display_width;
extern const int hardware_display_height;
} // namespace flx::config

namespace flx::hal {

/**
 * @brief Global hardware capability query interface.
 *
 * Blends compile-time YAML profile constraints with runtime DeviceRegistry state.
 * Solves the Tactility problem of "UI components crashing when a peripheral is disconnected".
 *
 * Example Usage:
 *   if (flx::hal::getCapabilities().hasGps()) {
 *       // Display satellite icon
 *   }
 */
struct HardwareCapabilities {
	// ── Peripherals ───────────────────────────────────────────────────────
	bool hasDisplay() const;
	bool hasTouch() const;
	bool hasSdCard() const;
	bool hasBattery() const; ///< true if Power PMIC or ADC is configured
	bool hasKeyboard() const;
	bool hasGps() const;
	bool hasUsb() const;

	// ── Characteristics ───────────────────────────────────────────────────
	uint16_t displayWidth() const;
	uint16_t displayHeight() const;

	// ── Chip Info ─────────────────────────────────────────────────────────
	const char* chipModel() const;
	uint32_t flashSizeBytes() const;
	uint32_t psramSizeBytes() const;
};

/**
 * @brief Obtain compile-time constraints based purely on profile.yaml.
 * Useful for fast GUI instantiation.
 */
constexpr HardwareCapabilities getCapabilities() {
	return HardwareCapabilities(); // Implementation delegated to .cpp for IDF links
}

/**
 * @brief Obtain active runtime capabilities.
 * Queries DeviceRegistry. Evaluates to false if a device is registered but in Error/Stopped state.
 */
HardwareCapabilities getActiveCapabilities();

} // namespace flx::hal
