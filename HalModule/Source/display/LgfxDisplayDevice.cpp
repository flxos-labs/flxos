#include <cinttypes>
#include <flx/core/Logger.hpp>
#include <flx/hal/display/LgfxDisplayDevice.hpp>

static constexpr const char* TAG = "LgfxDisplay";

#if !CONFIG_FLXOS_HEADLESS_MODE
#include "Config.hpp"
#include "display/lv_display.h"
#include "esp_heap_caps.h"
#include "lgfx/v1/lgfx_fonts.hpp"
#include "src/drivers/display/lovyan_gfx/lv_lovyan_gfx.h"
#endif

namespace flx::hal::display {

LgfxDisplayDevice::LgfxDisplayDevice() {
	setState(State::Uninitialized);
}

LgfxDisplayDevice::~LgfxDisplayDevice() {
	if (getState() == State::Ready) {
		stop();
	}
}

bool LgfxDisplayDevice::start() {
	setState(State::Starting);

#if !CONFIG_FLXOS_HEADLESS_MODE

	// ── 1. Create LovyanGFX instance ──────────────────────────────────────
	m_tft = std::make_unique<LGFX>();

	// ── 2. Smart DMA buffer allocation (plan §21) ─────────────────────────
	const uint32_t width = static_cast<uint32_t>(flx::config::display.width);
	const uint32_t height = static_cast<uint32_t>(flx::config::display.height);
	const size_t idealSize = static_cast<size_t>(width * height / 10) * 2; // 10% of screen in 16-bit

	size_t dmaFree = heap_caps_get_free_size(MALLOC_CAP_DMA);
	size_t bufSize = idealSize;

	if (dmaFree < idealSize * 2) {
		bufSize = idealSize / 2;
		flx::Log::warn(TAG, "Low DMA memory (%" PRIu32 " free), using smaller display buffer", (uint32_t)dmaFree);
	}
	if (dmaFree < idealSize) {
		bufSize = idealSize / 4;
		flx::Log::warn(TAG, "Very low DMA memory! Degrading further to %" PRIu32 " bytes", (uint32_t)bufSize);
	}

	m_bufferInfo.dmaFreeAtInit = dmaFree;

	m_dmaBuffer = heap_caps_malloc(bufSize, MALLOC_CAP_DMA);
	if (!m_dmaBuffer) {
		flx::Log::error(TAG, "DMA buffer allocation failed (requested %" PRIu32 " bytes)!", (uint32_t)bufSize);
		setState(State::Error);
		return false;
	}

	m_bufferInfo.bufferSize = bufSize;
	m_bufferInfo.doubleBuffered = false;

	flx::Log::info(TAG, "DMA buffer: %" PRIu32 " bytes (%.1f%% of %" PRIu32 "x%" PRIu32 " screen)", (uint32_t)bufSize, (float)bufSize / (float)(width * height * 2) * 100.0f, width, height);

	// ── 3. Create LVGL display via LovyanGFX bridge ───────────────────────
	bool touch_en = flx::config::touch.enabled;
	m_lvDisplay = lv_lovyan_gfx_create(
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height),
		m_dmaBuffer,
		bufSize,
		touch_en
	);

	if (!m_lvDisplay) {
		flx::Log::error(TAG, "lv_lovyan_gfx_create() failed!");
		heap_caps_free(m_dmaBuffer);
		m_dmaBuffer = nullptr;
		setState(State::Error);
		return false;
	}

	// ── 4. Release LGFX bus lock left open by lv_lovyan_gfx_create ────────
	m_tft->endWrite();

	// ── 5. Apply initial rotation from profile config ─────────────────────
	const int rotation = flx::config::display.rotation;
	lv_display_set_rotation(m_lvDisplay, static_cast<lv_display_rotation_t>(rotation / 90));
	m_rotation.set(static_cast<uint8_t>(rotation));

	// ── 6. Wire brightness observable → hardware ──────────────────────────
	m_brightness.subscribe([this](const uint8_t& duty) {
		if (m_tft) {
			m_tft->setBrightness(duty);
			m_backlightDuty = duty;
		}
	});

	flx::Log::info(TAG, "Display ready: %" PRIu32 "x%" PRIu32 ", rotation=%d°", width, height, rotation);

	setState(State::Ready);
	return true;

#else // CONFIG_FLXOS_HEADLESS_MODE

	flx::Log::info(TAG, "Headless mode — display driver not initialized");
	// In headless mode, LgfxDisplayDevice shouldn't normally be instantiated;
	// HeadlessDisplayDevice should be used instead. But set to Ready so registry works.
	setState(State::Ready);
	return true;

#endif
}

bool LgfxDisplayDevice::stop() {
#if !CONFIG_FLXOS_HEADLESS_MODE
	if (m_lvDisplay) {
		lv_display_delete(m_lvDisplay);
		m_lvDisplay = nullptr;
	}
	if (m_dmaBuffer) {
		heap_caps_free(m_dmaBuffer);
		m_dmaBuffer = nullptr;
	}
	m_tft.reset();
#endif
	setState(State::Stopped);
	return true;
}

// ── IDevice identity ──────────────────────────────────────────────────────

std::string_view LgfxDisplayDevice::getDescription() const {
#if !CONFIG_FLXOS_HEADLESS_MODE
	return "LovyanGFX display driver (YAML-configured, DMA-backed)";
#else
	return "LovyanGFX display driver (headless stub)";
#endif
}

// ── IDisplayDevice ────────────────────────────────────────────────────────

uint16_t LgfxDisplayDevice::getWidth() const {
#if !CONFIG_FLXOS_HEADLESS_MODE
	return static_cast<uint16_t>(flx::config::display.width);
#else
	return 0;
#endif
}

uint16_t LgfxDisplayDevice::getHeight() const {
#if !CONFIG_FLXOS_HEADLESS_MODE
	return static_cast<uint16_t>(flx::config::display.height);
#else
	return 0;
#endif
}

ColorFormat LgfxDisplayDevice::getColorFormat() const {
	return ColorFormat::RGB565; // LovyanGFX defaults to RGB565 for ESP32
}

bool LgfxDisplayDevice::supportsBacklight() const {
#if !CONFIG_FLXOS_HEADLESS_MODE
	return flx::config::display.pins.bckl != -1;
#else
	return false;
#endif
}

void LgfxDisplayDevice::setBacklightDuty(uint8_t duty) {
	m_brightness.set(duty); // Observable notifies the subscriber wired in start()
}

uint8_t LgfxDisplayDevice::getBacklightDuty() const {
	return m_brightness.get();
}

struct _lv_display_t* LgfxDisplayDevice::getLvglDisplay() const {
	return m_lvDisplay;
}

std::shared_ptr<flx::hal::touch::ITouchDevice> LgfxDisplayDevice::getAttachedTouch() const {
	return m_touch;
}

void LgfxDisplayDevice::runColorTest(uint32_t color) {
#if !CONFIG_FLXOS_HEADLESS_MODE
	if (!m_tft) return;
	m_tft->clear(static_cast<uint32_t>(color));
	m_tft->setTextDatum(middle_center);
	m_tft->setTextColor(0xFFFFFF - color); // Invert text color against background
	m_tft->setFont(&Font4);
	m_tft->drawCentreString("Low Level Driver", m_tft->width() / 2, m_tft->height() / 2);
	m_tft->drawCentreString("HAL Display Test", m_tft->width() / 2, (m_tft->height() / 2) + (m_tft->fontHeight() * 2));
	flx::Log::info(TAG, "Color test executed (color=0x%" PRIX32 ")", color);
#else
	flx::Log::warn(TAG, "runColorTest() not available in headless mode");
#endif
}

} // namespace flx::hal::display
