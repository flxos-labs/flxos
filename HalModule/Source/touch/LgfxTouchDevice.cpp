#include <flx/core/Logger.hpp>
#include <flx/hal/touch/LgfxTouchDevice.hpp>

// Include LVGL and LovyanGFX headers if not running headed mode
#if !CONFIG_FLXOS_HEADLESS_MODE
#include "Config.hpp"
#include "display/lv_display.h"
#include "indev/lv_indev.h"
#include "src/drivers/display/lovyan_gfx/lv_lovyan_gfx.h"
#include <flx/hal/lv_lgfx_user.hpp>
#endif

static constexpr const char* TAG = "LgfxTouch";

namespace flx::hal::touch {

#if !CONFIG_FLXOS_HEADLESS_MODE
LgfxTouchDevice::LgfxTouchDevice(LGFX* tft) : m_tft(tft) {
	this->setState(State::Uninitialized);
}
#else
LgfxTouchDevice::LgfxTouchDevice() {
	this->setState(State::Uninitialized);
}
#endif

LgfxTouchDevice::~LgfxTouchDevice() {
	if (getState() == State::Ready) {
		stop();
	}
}

bool LgfxTouchDevice::start() {
	this->setState(State::Starting);
#if !CONFIG_FLXOS_HEADLESS_MODE
	if (!m_tft) {
		flx::Log::error(TAG, "Cannot start touch device: LGFX missing!");
		this->setState(State::Error);
		return false;
	}

	// Touch initialization is handled implicitly by LGFX initialization logic.
	// We just query calibration values here.
	auto cal = m_tft->touch();
	if (cal) {
		flx::Log::info(TAG, "Touch initialized properly and calibration retrieved.");
	} else {
		flx::Log::warn(TAG, "LGFX touch module not present or inactive.");
	}
#else
	flx::Log::info(TAG, "Headless touch started (stub).");
#endif
	this->setState(State::Ready);
	return true;
}

bool LgfxTouchDevice::stop() {
	this->setState(State::Stopped);
	return true;
}

bool LgfxTouchDevice::getTouchedPoints(uint16_t* x, uint16_t* y, uint8_t* pointCount, uint8_t maxPoints) {
#if !CONFIG_FLXOS_HEADLESS_MODE
	if (m_tft && maxPoints > 0) {
		bool touched = m_tft->getTouch(x, y);
		if (pointCount) *pointCount = touched ? 1 : 0;
		return touched;
	}
	return false;
#else
	return false;
#endif
}

struct _lv_indev_t* LgfxTouchDevice::getLvglIndev() const {
#if !CONFIG_FLXOS_HEADLESS_MODE
	// Naively return the first LV_INDEV_TYPE_POINTER
	lv_indev_t* indev = lv_indev_get_next(nullptr);
	while (indev) {
		if (lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER) {
			return indev;
		}
		indev = lv_indev_get_next(indev);
	}
	return nullptr;
#else
	return nullptr;
#endif
}

ITouchDevice::CalibrationData LgfxTouchDevice::getCalibration() const {
	return m_calibration;
}

void LgfxTouchDevice::setCalibration(const ITouchDevice::CalibrationData& cal) {
	m_calibration = cal;
#if !CONFIG_FLXOS_HEADLESS_MODE
	if (m_tft) {
		uint16_t calData[8] = {
			static_cast<uint16_t>(cal.xMin),
			static_cast<uint16_t>(cal.yMin),
			static_cast<uint16_t>(cal.xMax),
			static_cast<uint16_t>(cal.yMin),
			static_cast<uint16_t>(cal.xMin),
			static_cast<uint16_t>(cal.yMax),
			static_cast<uint16_t>(cal.xMax),
			static_cast<uint16_t>(cal.yMax)
		};
		m_tft->setTouchCalibrate(calData);
	}
#endif
}

} // namespace flx::hal::touch
