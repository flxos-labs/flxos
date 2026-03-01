#pragma once

#include <cstdint>
#include <flx/hal/IDevice.hpp>
#include <memory>

// Forward declarations to avoid circular includes
namespace flx::hal::touch {
class ITouchDevice;
}

// Forward declare LVGL display handle in the global namespace
struct _lv_display_t;

namespace flx::hal::display {

/**
 * @brief Pixel color format supported by the display.
 */
enum class ColorFormat : uint8_t {
	Monochrome,
	RGB565,
	RGB565Swapped,
	BGR565,
	BGR565Swapped,
	RGB888
};

/**
 * @brief Abstract interface for display hardware devices.
 *
 * Implemented by LgfxDisplayDevice (LovyanGFX-backed) and
 * HeadlessDisplayDevice (no-op stub for headless builds).
 *
 * Key improvements over Tactility's DisplayDevice:
 *  - Power control API (Tactility: missing)
 *  - Screenshot capture (Tactility: missing)
 *  - Bitmap draw API (Tactility: missing)
 *  - LVGL display handle exposed directly
 *  - Observable brightness/rotation via DeviceBase pattern
 *  - DMA buffer info query
 */
class IDisplayDevice : public flx::hal::IDevice {
public:

	// ── IDevice ───────────────────────────────────────────────────────────
	Type getType() const override { return Type::Display; }

	// ── Display geometry ──────────────────────────────────────────────────
	virtual uint16_t getWidth() const = 0;
	virtual uint16_t getHeight() const = 0;
	virtual ColorFormat getColorFormat() const = 0;

	// ── Backlight ─────────────────────────────────────────────────────────
	virtual bool supportsBacklight() const = 0;
	/** Set backlight brightness [0–255]. 0 = off, 255 = max. */
	virtual void setBacklightDuty(uint8_t duty) = 0;
	virtual uint8_t getBacklightDuty() const = 0;

	// ── Power control (surpasses Tactility) ───────────────────────────────
	virtual bool supportsPowerControl() const { return false; }
	virtual void setPowerOn(bool on) { (void)on; }
	virtual bool isPoweredOn() const { return true; }

	// ── E-paper full refresh (passes Tactility) ───────────────────────────
	/** Request a full (non-partial) refresh for e-paper displays. */
	virtual void requestFullRefresh() {}

	// ── Gamma (matches Tactility) ─────────────────────────────────────────
	virtual void setGammaCurve(uint8_t index) { (void)index; }
	virtual uint8_t getGammaCurveCount() const { return 0; }

	// ── LVGL integration ──────────────────────────────────────────────────
	/**
     * @brief Get the LVGL display handle.
     * Created during start() and destroyed during stop().
     * May return nullptr if the device is not in State::Ready.
     */
	virtual struct _lv_display_t* getLvglDisplay() const = 0;

	// ── Touch association ─────────────────────────────────────────────────
	/**
     * @brief Get the touch device logically attached to this display.
     * Returns nullptr if no touch device is associated.
     */
	virtual std::shared_ptr<flx::hal::touch::ITouchDevice> getAttachedTouch() const = 0;

	// ── Raw pixel access (surpasses Tactility) ────────────────────────────
	/**
     * @brief Draw raw pixel data directly to the display (bypasses LVGL).
     * Useful for screenshot restore or framebuffer operations.
     * @return true if supported and successful, false otherwise.
     */
	virtual bool drawBitmap(int xStart, int yStart, int xEnd, int yEnd, const void* pixelData) {
		(void)xStart;
		(void)yStart;
		(void)xEnd;
		(void)yEnd;
		(void)pixelData;
		return false;
	}

	// ── Display test (surpasses Tactility) ────────────────────────────────
	/** Fill the screen with a solid color for hardware validation. */
	virtual void runColorTest(uint32_t color) { (void)color; }

	// ── Screenshot (surpasses Tactility) ──────────────────────────────────
	/**
     * @brief Capture a screenshot into a caller-provided buffer.
     * @param buffer      Output buffer (must hold width × height × bytes_per_pixel).
     * @param bufferSize  Size of buffer in bytes.
     * @return true if screenshot was captured, false if unsupported.
     */
	virtual bool captureScreenshot(void* buffer, size_t bufferSize) {
		(void)buffer;
		(void)bufferSize;
		return false;
	}

	// ── DMA buffer info (surpasses Tactility) ─────────────────────────────
	struct BufferInfo {
		size_t bufferSize = 0; ///< Active DMA buffer size in bytes
		size_t dmaFreeAtInit = 0; ///< Free DMA heap at device init time
		bool doubleBuffered = false; ///< Whether double-buffering is active
	};
	virtual BufferInfo getBufferInfo() const { return {}; }
};

} // namespace flx::hal::display
