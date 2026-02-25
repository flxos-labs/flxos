#pragma once

#include <cstdint>
#include <flx/hal/IDevice.hpp>
#include <functional>

// Forward declare LVGL handle to avoid pulling in all LVGL headers
struct _lv_indev_t;

namespace flx::hal::input {

/**
 * @brief Abstract interface for input devices like Keyboards, Encoders, Buttons.
 *
 * Implemented by I2C keypads (TCA8418), rotary encoders, trackballs, etc.
 *
 * Key improvements over Tactility's separate Keyboard/Encoder devices:
 *  - Unified input hierarchy.
 *  - Raw key event subscription (Tactility only feeds LVGL indevs implicitly).
 *  - Supports multiple input types (Trackball added).
 */
class IInputDevice : public flx::hal::IDevice {
public:

	enum class InputType : uint8_t {
		Keyboard,
		Encoder,
		Button,
		Trackball
	};

	// ── Input properties ──────────────────────────────────────────────────
	virtual InputType getInputType() const = 0;
	virtual bool isAttached() const { return true; }

	// ── LVGL integration ──────────────────────────────────────────────────
	/**
     * @brief Get the associated LVGL input device handle.
     * Often an LV_INDEV_TYPE_KEYPAD or LV_INDEV_TYPE_ENCODER.
     */
	virtual struct _lv_indev_t* getLvglIndev() const = 0;

	// ── Raw Event Subscription ────────────────────────────────────────────
	struct KeyEvent {
		uint32_t keyCode; ///< Key index or ASCII character
		bool pressed; ///< true = press down, false = release
		uint32_t timestamp; ///< System tick timestamp (ms)
	};

	using KeyEventCallback = std::function<void(const KeyEvent&)>;

	/**
     * @brief Subscribe to raw hardware key events (before LVGL processing).
     * Useful for global hotkeys or non-GUI services.
     */
	virtual int subscribeKeyEvents(KeyEventCallback cb) {
		(void)cb;
		return -1;
	}
	virtual void unsubscribeKeyEvents(int id) { (void)id; }
};

// ── Convenience Subinterfaces ─────────────────────────────────────────────

/** Base interface for Keyboard/Keypad devices. */
class IKeyboardDevice : public IInputDevice {
public:

	Type getType() const override { return Type::Keyboard; }
	InputType getInputType() const override { return InputType::Keyboard; }
};

/** Base interface for Rotary Encoder devices. */
class IEncoderDevice : public IInputDevice {
public:

	Type getType() const override { return Type::Encoder; }
	InputType getInputType() const override { return InputType::Encoder; }
};

} // namespace flx::hal::input
