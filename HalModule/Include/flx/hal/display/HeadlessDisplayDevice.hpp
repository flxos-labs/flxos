#pragma once

#include <flx/hal/DeviceBase.hpp>
#include <flx/hal/display/IDisplayDevice.hpp>

namespace flx::hal::display {

/**
 * @brief No-op display device for headless / CI builds.
 *
 * Satisfies all IDisplayDevice queries with safe defaults.
 * Surpasses Tactility — Tactility has NO headless build support.
 *
 * Registered by HalInitService when CONFIG_FLXOS_HEADLESS_MODE=1.
 * This allows all system services that query DeviceRegistry for a display
 * to compile and run without any LovyanGFX or LVGL dependency.
 */
class HeadlessDisplayDevice final : public flx::hal::DeviceBase<IDisplayDevice> {
public:

	// ── IDevice identity ──
	std::string_view getName() const override { return "Headless Display"; }
	std::string_view getDescription() const override { return "No-op stub for headless/CI builds"; }
	Type getType() const override { return IDisplayDevice::getType(); }
	Id getId() const override { return DeviceBase::getId(); }
	State getState() const override { return DeviceBase::getState(); }

	// ── Lifecycle ──
	bool start() override {
		setState(State::Ready);
		return true;
	}

	bool stop() override {
		setState(State::Stopped);
		return true;
	}

	// ── IDisplayDevice ──
	uint16_t getWidth() const override { return 0; }
	uint16_t getHeight() const override { return 0; }
	ColorFormat getColorFormat() const override { return ColorFormat::RGB565; }

	bool supportsBacklight() const override { return false; }
	void setBacklightDuty(uint8_t) override {}
	uint8_t getBacklightDuty() const override { return 0; }

	struct _lv_display_t* getLvglDisplay() const override { return nullptr; }
	std::shared_ptr<flx::hal::touch::ITouchDevice> getAttachedTouch() const override { return nullptr; }
};

} // namespace flx::hal::display
