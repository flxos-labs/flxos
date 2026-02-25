#pragma once

#include <cstdint>
#include <flx/hal/IDevice.hpp>

// Forward declare LVGL handle to avoid pulling in all LVGL headers
struct _lv_indev_t;

namespace flx::hal::touch {

/**
 * @brief Abstract interface for touch input devices.
 *
 * Implemented by LgfxTouchDevice (LovyanGFX-backed).
 *
 * Key improvements over Tactility's TouchDevice:
 *  - Gesture support API (Tactility: missing in most drivers)
 *  - Runtime calibration via setCalibration() (Tactility: no runtime recal)
 *  - Multi-touch point API (Tactility: single-touch everywhere)
 *  - Integrated with display association model
 */
class ITouchDevice : public flx::hal::IDevice {
public:

	// ── IDevice ───────────────────────────────────────────────────────────
	Type getType() const override { return Type::Touch; }

	// ── Raw touch input ───────────────────────────────────────────────────
	/**
     * @brief Read current touched points.
     * @param x          Output array of X coordinates.
     * @param y          Output array of Y coordinates.
     * @param pointCount Output: number of active touch points (≤ maxPoints).
     * @param maxPoints  Maximum results to write into x/y arrays.
     * @return true if any touch is active.
     */
	virtual bool getTouchedPoints(uint16_t* x, uint16_t* y, uint8_t* pointCount, uint8_t maxPoints) = 0;

	// ── Multi-touch ───────────────────────────────────────────────────────
	/**
     * @brief Maximum simultaneous touch points this device supports.
     * Most resistive panels: 1. Capacitive GT911: 5. RST others: 2–10.
     * Surpasses Tactility — Tactility returns 1 for all drivers.
     */
	virtual uint8_t getMaxTouchPoints() const { return 1; }

	// ── Gesture support (surpasses Tactility) ─────────────────────────────
	enum class Gesture : uint8_t {
		None,
		SwipeUp,
		SwipeDown,
		SwipeLeft,
		SwipeRight,
		Pinch,
		Zoom
	};
	virtual bool supportsGestures() const { return false; }
	virtual Gesture getLastGesture() { return Gesture::None; }

	// ── LVGL integration ──────────────────────────────────────────────────
	/**
     * @brief LVGL input device handle.
     * GuiTask uses this to feed touch events into LVGL.
     * May return nullptr if the device hasn't started yet.
     */
	virtual struct _lv_indev_t* getLvglIndev() const = 0;

	// ── Runtime calibration (surpasses Tactility) ─────────────────────────
	/**
     * @brief Touch calibration parameters.
     * xMin/xMax/yMin/yMax: raw ADC bounds for resistive panels.
     * offsetRotation: pre-rotation offset to align touch with display.
     */
	struct CalibrationData {
		int16_t xMin = 0;
		int16_t xMax = 4095;
		int16_t yMin = 0;
		int16_t yMax = 4095;
		uint8_t offsetRotation = 0;
	};

	virtual CalibrationData getCalibration() const = 0;

	/**
     * @brief Update touch calibration at runtime.
     * Persist the result to NVS via SettingsManager if needed.
     * Tactility has no runtime recalibration support.
     */
	virtual void setCalibration(const CalibrationData& cal) { (void)cal; }
};

} // namespace flx::hal::touch
