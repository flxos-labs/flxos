#pragma once

#include <cstdint>
#include <flx/hal/IDevice.hpp>
#include <functional>

namespace flx::hal::gpio {

using Pin = int;
constexpr Pin PIN_NC = -1; ///< Not Connected / invalid pin

/**
 * @brief GPIO pin operational mode.
 */
enum class GpioMode : uint8_t {
	Disable,
	Input,
	Output,
	OutputOpenDrain,
	InputOutput,
	InputOutputOpenDrain
};

/**
 * @brief GPIO pull resistor mode.
 */
enum class GpioPullMode : uint8_t {
	None,
	Up,
	Down,
	Both
};

/**
 * @brief ISR trigger edge.
 */
enum class GpioInterruptEdge : uint8_t {
	None,
	Rising,
	Falling,
	Both
};

/**
 * @brief Abstract interface for GPIO controller devices.
 *
 * Key improvements over Tactility's GPIO namespace functions:
 *  - Object-oriented, registered in DeviceRegistry
 *  - Hardware interrupt (ISR) support with callbacks
 *  - Software debouncing for button inputs
 *  - Pin count introspection
 *
 * Tactility has gpio_get/gpio_set free functions with no controller object.
 * FlxOS encapsulates GPIO into a device that can be health-checked and observed.
 */
class IGpioController : public flx::hal::IDevice {
public:

	// ── IDevice ───────────────────────────────────────────────────────────
	Type getType() const override { return Type::Gpio; }

	// ── Configuration ─────────────────────────────────────────────────────
	/**
     * @brief Configure a GPIO pin's mode and pull resistors.
     * @return true if configured successfully.
     */
	virtual bool configure(Pin pin, GpioMode mode, GpioPullMode pull = GpioPullMode::None) = 0;

	/**
     * @brief Set a GPIO output level.
     * @param level  true = high, false = low.
     * @return true if set successfully.
     */
	virtual bool setLevel(Pin pin, bool level) = 0;

	/**
     * @brief Read a GPIO input level.
     * @return true if high, false if low.
     */
	virtual bool getLevel(Pin pin) const = 0;

	/**
     * @brief Number of GPIO pins this controller manages.
     */
	virtual int getPinCount() const = 0;

	// ── Interrupts (surpasses Tactility) ──────────────────────────────────
	/**
	 * Callback type invoked when a GPIO edge is detected.
	 * The callback is dispatched from a normal FreeRTOS task context (not from
	 * the ISR itself), so heap allocation and blocking APIs are safe to use.
	 * The underlying ISR only enqueues the pin number and returns immediately.
	 */
	using GpioCallback = std::function<void(Pin pin, bool level)>;

	/**
     * @brief Attach an interrupt callback to a GPIO pin.
     * The callback is dispatched from a task after the edge is detected —
     * it is NOT called directly from ISR context.
     * @return true if attached successfully.
     */
	virtual bool attachInterrupt(Pin pin, GpioInterruptEdge edge, GpioCallback callback) {
		(void)pin;
		(void)edge;
		(void)callback;
		return false;
	}

	/**
     * @brief Detach an interrupt from a GPIO pin.
     * @return true if detached successfully.
     */
	virtual bool detachInterrupt(Pin pin) {
		(void)pin;
		return false;
	}

	// ── Debounced input (surpasses Tactility) ─────────────────────────────
	/**
     * @brief Configure a debounced input with a callback.
     * The callback fires from task context after the level is stable for debounceMs.
     * @return true if configured successfully.
     */
	virtual bool configureDebounced(Pin pin, uint32_t debounceMs, GpioCallback callback) {
		(void)pin;
		(void)debounceMs;
		(void)callback;
		return false;
	}
};

} // namespace flx::hal::gpio
