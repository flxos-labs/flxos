#pragma once

#include <cstdint>

// Legacy variables removed. Access flx::config structures directly in .cpp instead.

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
	bool hasWifi() const;
	bool hasBluetooth() const;
	bool hasI2C() const;
	bool hasSpi() const;
	bool hasUart() const;
	bool hasGpio() const;

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
