#pragma once

#include <cstdint>
#include <string_view>

namespace flx::hal {

/**
 * @brief Base interface for all hardware devices in the FlxOS HAL.
 *
 * Every device (display, touch, SD card, GPIO controller, etc.) implements
 * this interface to participate in the DeviceRegistry and HAL lifecycle.
 *
 * State machine:
 *   Uninitialized → Starting → Ready
 *                            ↘ Error
 *   Ready → Stopped
 */
class IDevice {
public:

	using Id = uint32_t;

	/**
     * @brief Broad hardware category of the device.
     * Used for typed registry queries (findFirst<IDisplayDevice>(Type::Display)).
     */
	enum class Type : uint8_t {
		Display,
		Touch,
		SdCard,
		Gpio,
		I2c,
		Spi,
		Uart,
		Power,
		Keyboard,
		Encoder,
		Usb,
		Sensor,
		Gps,
		Other
	};

	/**
     * @brief Lifecycle state of the device.
     * Tactility has NO equivalent — devices are either started or not.
     */
	enum class State : uint8_t {
		Uninitialized,
		Starting,
		Ready,
		Error,
		Stopped
	};

	virtual ~IDevice() = default;

	// ── Identity ──────────────────────────────────────────────────────────
	/** Unique runtime ID, auto-assigned by DeviceBase. */
	virtual Id getId() const = 0;
	/** Broad hardware category. */
	virtual Type getType() const = 0;
	/** Short human-readable name (e.g. "LGFX Display"). */
	virtual std::string_view getName() const = 0;
	/** Longer description including driver and config details. */
	virtual std::string_view getDescription() const = 0;

	// ── Lifecycle ─────────────────────────────────────────────────────────
	/**
     * @brief Initialize and start the device.
     * @return true if the device reached State::Ready, false on error.
     */
	virtual bool start() = 0;

	/**
     * @brief Shut down the device cleanly.
     * @return true if stopped gracefully.
     */
	virtual bool stop() = 0;

	/** Current lifecycle state. Thread-safe (backed by std::atomic). */
	virtual State getState() const = 0;

	// ── Health ────────────────────────────────────────────────────────────
	/**
     * @brief Convenience health query.
     * Surpasses Tactility — Tactility has no device health monitoring.
     */
	virtual bool isHealthy() const { return getState() == State::Ready; }

	// ── Helpers ───────────────────────────────────────────────────────────
	/** Convert Type enum to human-readable C-string. */
	static constexpr const char* typeToString(Type t) {
		switch (t) {
			case Type::Display:
				return "Display";
			case Type::Touch:
				return "Touch";
			case Type::SdCard:
				return "SdCard";
			case Type::Gpio:
				return "GPIO";
			case Type::I2c:
				return "I2C";
			case Type::Spi:
				return "SPI";
			case Type::Uart:
				return "UART";
			case Type::Power:
				return "Power";
			case Type::Keyboard:
				return "Keyboard";
			case Type::Encoder:
				return "Encoder";
			case Type::Usb:
				return "USB";
			case Type::Sensor:
				return "Sensor";
			case Type::Gps:
				return "GPS";
			default:
				return "Other";
		}
	}

	/** Convert State enum to human-readable C-string. */
	static constexpr const char* stateToString(State s) {
		switch (s) {
			case State::Uninitialized:
				return "Uninitialized";
			case State::Starting:
				return "Starting";
			case State::Ready:
				return "Ready";
			case State::Error:
				return "Error";
			case State::Stopped:
				return "Stopped";
			default:
				return "Unknown";
		}
	}
};

} // namespace flx::hal
