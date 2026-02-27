#pragma once

#include <flx/core/Observable.hpp>
#include <flx/hal/DeviceBase.hpp>
#include <flx/hal/display/IDisplayDevice.hpp>
#include <memory>

// LovyanGFX is only available when not in headless mode
#if !CONFIG_FLXOS_HEADLESS_MODE
#include <flx/hal/lv_lgfx_user.hpp>
#endif

namespace flx::hal::touch {
class ITouchDevice;
}

namespace flx::hal::display {

/**
 * @brief Concrete LovyanGFX-backed display device.
 *
 * Wraps the existing LGFX class (from lv_lgfx_user.hpp) and the
 * lv_lovyan_gfx_create() LVGL integration into a proper HAL device.
 *
 * Key advantages over the current GuiTask approach:
 *  - Registered with DeviceRegistry — code can discover the display at runtime
 *  - Observable brightness/rotation — UI subscribes, not polls
 *  - Smart DMA buffer allocation (degrades gracefully on low memory)
 *  - State tracking (Ready / Error / Stopped)
 *  - runColorTest() is part of the device, not scattered in GuiTask
 *  - Headless-transparent: code that uses IDisplayDevice doesn't need #ifdef guards
 *
 * This single class handles ALL 40+ display panels supported by LovyanGFX via
 * the compile-time flx::config::* constexpr dispatch in lv_lgfx_user.hpp.
 * Tactility requires a separate driver package per display chip.
 */
class LgfxDisplayDevice final : public flx::hal::DeviceBase<IDisplayDevice> {
public:

	LgfxDisplayDevice();
	~LgfxDisplayDevice() override;

	// ── IDevice identity ──────────────────────────────────────────────────
	std::string_view getName() const override { return "LGFX Display"; }
	std::string_view getDescription() const override;
	Type getType() const override { return IDisplayDevice::getType(); }
	// ── Lifecycle ─────────────────────────────────────────────────────────
	/**
     * @brief Initialize LovyanGFX, allocate DMA buffer, create LVGL display.
     *
     * DMA buffer strategy (Smart Allocation from plan §21):
     *   Ideal  = width × height / 10 × 2 bytes  (10% of screen)
     *   If DMA < ideal×2: use ideal/2  (5% of screen)
     *   If DMA < ideal:   use ideal/4  (2.5% of screen)
     *   If allocation fails: setState(Error), return false
     *
     * @return true if display reached State::Ready.
     */
	bool start() override;

	/**
     * @brief Destroy LVGL display, free DMA buffer, reset LGFX instance.
     */
	bool stop() override;

	// ── IDisplayDevice ────────────────────────────────────────────────────
	uint16_t getWidth() const override;
	uint16_t getHeight() const override;
	ColorFormat getColorFormat() const override;

	bool supportsBacklight() const override;
	void setBacklightDuty(uint8_t duty) override;
	uint8_t getBacklightDuty() const override;

	struct _lv_display_t* getLvglDisplay() const override;

	std::shared_ptr<flx::hal::touch::ITouchDevice> getAttachedTouch() const override;

	void runColorTest(uint32_t color) override;

	BufferInfo getBufferInfo() const override { return m_bufferInfo; }

	// ── Observable properties ─────────────────────────────────────────────
	/**
     * @brief Observable brightness [0–255].
     * DisplayManager subscribes to this instead of calling setBacklightDuty directly.
     * Surpasses Tactility — no observable pattern on Tactility devices.
     */
	flx::Observable<uint8_t>& getBrightnessObservable() { return m_brightness; }

	/**
     * @brief Observable rotation (0, 90, 180, 270 degrees).
     */
	flx::Observable<uint8_t>& getRotationObservable() { return m_rotation; }

	// ── LovyanGFX raw access (for GuiTask compatibility) ──────────────────
#if !CONFIG_FLXOS_HEADLESS_MODE
	/** Direct access to the LGFX driver. Use sparingly — prefer interface methods. */
	LGFX* getRawDriver() const { return m_tft; }
#endif

private:

#if !CONFIG_FLXOS_HEADLESS_MODE
	LGFX* m_tft = nullptr; ///< LovyanGFX driver instance
	void* m_dmaBuffer = nullptr; ///< DMA-capable display buffer
#endif

	struct _lv_display_t* m_lvDisplay = nullptr; ///< LVGL display handle

	uint8_t m_backlightDuty = 127; ///< Current backlight duty [0–255]
	flx::Observable<uint8_t> m_brightness {127};
	flx::Observable<uint8_t> m_rotation {0};

	BufferInfo m_bufferInfo; ///< DMA buffer stats (filled in start())

	std::shared_ptr<flx::hal::touch::ITouchDevice> m_touch; ///< Attached touch device
};

} // namespace flx::hal::display
