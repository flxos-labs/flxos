#pragma once

#include <flx/hal/DeviceBase.hpp>
#include <flx/hal/touch/ITouchDevice.hpp>
#include <memory>
#include <string_view>

#if !CONFIG_FLXOS_HEADLESS_MODE
class LGFX; // Forward declaration from LovyanGFX
#endif

namespace flx::hal::touch {

/**
 * @brief Concrete touch device implementation backed by LovyanGFX.
 *
 * This class wraps the touch capabilities of the LovyanGFX driver instance
 * that is created and managed by LgfxDisplayDevice.
 */
class LgfxTouchDevice final : public flx::hal::DeviceBase<ITouchDevice> {
public:

#if !CONFIG_FLXOS_HEADLESS_MODE
	/**
     * @brief Constructor.
     * @param tft  Raw pointer to the LovyanGFX driver instance managing this touch panel.
     */
	explicit LgfxTouchDevice(LGFX* tft);
#else
	LgfxTouchDevice();
#endif
	~LgfxTouchDevice() override;

	// ── IDevice ───────────────────────────────────────────────────────────
	std::string_view getName() const override { return "LGFX Touch"; }
	std::string_view getDescription() const override { return "LovyanGFX integrated touch driver"; }
	bool start() override;
	bool stop() override;

	// ── ITouchDevice ──────────────────────────────────────────────────────
	bool getTouchedPoints(uint16_t* x, uint16_t* y, uint8_t* pointCount, uint8_t maxPoints) override;
	uint8_t getMaxTouchPoints() const override { return 1; } // Most basic setups return 1.
	struct _lv_indev_t* getLvglIndev() const override;
	CalibrationData getCalibration() const override;
	void setCalibration(const CalibrationData& cal) override;

private:

#if !CONFIG_FLXOS_HEADLESS_MODE
	LGFX* m_tft; ///< Weak reference to the display driver handling the physical I2C/SPI touch logic.
#endif
	CalibrationData m_calibration;
};

} // namespace flx::hal::touch
