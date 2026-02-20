#pragma once

#define LGFX_USE_V1

#include <LovyanGFX.hpp>
#include <sdkconfig.h>

#include <Config.hpp>

#ifndef FLXOS_ENABLE_DISPLAY
#if defined(CONFIG_FLXOS_ENABLE_DISPLAY)
#define FLXOS_ENABLE_DISPLAY CONFIG_FLXOS_ENABLE_DISPLAY
#elif defined(FLXOS_DISPLAY_WIDTH) && defined(FLXOS_DISPLAY_HEIGHT)
#define FLXOS_ENABLE_DISPLAY true
#else
#define FLXOS_ENABLE_DISPLAY false
#endif
#endif

#ifndef FLXOS_BUS_SPI
#if defined(CONFIG_FLXOS_BUS_SPI)
#define FLXOS_BUS_SPI CONFIG_FLXOS_BUS_SPI
#endif
#endif

#ifndef FLXOS_DISPLAY_ILI9341
#if defined(CONFIG_FLXOS_DISPLAY_ILI9341)
#define FLXOS_DISPLAY_ILI9341 CONFIG_FLXOS_DISPLAY_ILI9341
#endif
#endif

#ifndef FLXOS_TOUCH_XPT2046
#if defined(CONFIG_FLXOS_TOUCH_XPT2046)
#define FLXOS_TOUCH_XPT2046 CONFIG_FLXOS_TOUCH_XPT2046
#endif
#endif

#if !FLXOS_ENABLE_DISPLAY
#error "FLXOS_ENABLE_DISPLAY must be enabled when using LGFX."
#endif

#if !FLXOS_BUS_SPI
#error "Only SPI display bus is currently supported by flx/hal/lv_lgfx_user.hpp."
#endif

#if !FLXOS_DISPLAY_ILI9341
#error "Only ILI9341 panel is currently supported by flx/hal/lv_lgfx_user.hpp."
#endif

#if FLXOS_TOUCH_ENABLED && !FLXOS_TOUCH_XPT2046
#error "Only XPT2046 touch is currently supported by flx/hal/lv_lgfx_user.hpp."
#endif

#ifndef FLXOS_SPI_3WIRE
#define FLXOS_SPI_3WIRE false
#endif
#ifndef FLXOS_SPI_DMA_CHANNEL
#define FLXOS_SPI_DMA_CHANNEL 0
#endif
#ifndef FLXOS_DISPLAY_INVERT
#define FLXOS_DISPLAY_INVERT false
#endif
#ifndef FLXOS_DISPLAY_RGB_ORDER
#define FLXOS_DISPLAY_RGB_ORDER false
#endif
#ifndef FLXOS_PANEL_DLEN_16BIT
#define FLXOS_PANEL_DLEN_16BIT false
#endif
#ifndef FLXOS_BCKL_INVERT
#define FLXOS_BCKL_INVERT false
#endif
#ifndef FLXOS_TOUCH_SPI_SEPARATE_PINS
#define FLXOS_TOUCH_SPI_SEPARATE_PINS false
#endif
#ifndef FLXOS_TOUCH_SPI_HOST
#define FLXOS_TOUCH_SPI_HOST -1
#endif
#ifndef FLXOS_PIN_TOUCH_SCLK
#define FLXOS_PIN_TOUCH_SCLK -1
#endif
#ifndef FLXOS_PIN_TOUCH_MOSI
#define FLXOS_PIN_TOUCH_MOSI -1
#endif
#ifndef FLXOS_PIN_TOUCH_MISO
#define FLXOS_PIN_TOUCH_MISO -1
#endif

// ============================================================================
// LovyanGFX Configuration Class
// ============================================================================
class LGFX : public lgfx::LGFX_Device {
	lgfx::Panel_ILI9341 _panel_instance;
	lgfx::Bus_SPI _bus_instance;
	lgfx::Light_PWM _light_instance;
	lgfx::Touch_XPT2046 _touch_instance;

public:

	// Explicitly expose base methods
	int32_t width() const { return lgfx::LGFX_Device::width(); }
	int32_t height() const { return lgfx::LGFX_Device::height(); }
	uint8_t getRotation() const { return lgfx::LGFX_Device::getRotation(); }
	void setRotation(uint8_t r) { lgfx::LGFX_Device::setRotation(r); }

	void test_inheritance() {
		volatile int w = this->width();
		(void)w;
	}
#pragma message "Building LGFX User Header - LGFX Class Defined (Config.hpp)"

	LGFX(void) {
		// ====================================================================
		// Bus Configuration (SPI)
		// ====================================================================
		{
			auto cfg = _bus_instance.config();
			cfg.spi_host = FLXOS_SPI_HOST;
			cfg.spi_mode = FLXOS_SPI_MODE;
			cfg.freq_write = FLXOS_SPI_FREQ_WRITE;
			cfg.freq_read = FLXOS_SPI_FREQ_READ;
			cfg.spi_3wire = FLXOS_SPI_3WIRE;
			cfg.use_lock = true;
			cfg.dma_channel =
				(FLXOS_SPI_DMA_CHANNEL == 0) ? SPI_DMA_CH_AUTO :
				(FLXOS_SPI_DMA_CHANNEL == 1) ? 1 :
				(FLXOS_SPI_DMA_CHANNEL == 2) ? 2 : SPI_DMA_CH_AUTO;
			cfg.pin_sclk = FLXOS_PIN_SCLK;
			cfg.pin_mosi = FLXOS_PIN_MOSI;
			cfg.pin_miso = FLXOS_PIN_MISO;
			cfg.pin_dc = FLXOS_PIN_DC;
			_bus_instance.config(cfg);
			_panel_instance.setBus(&_bus_instance);
		}

		// ====================================================================
		// Panel Configuration (ILI9341)
		// ====================================================================
		{
			auto cfg = _panel_instance.config();
			cfg.pin_cs = FLXOS_PIN_CS;
			cfg.pin_rst = FLXOS_PIN_RST;
			cfg.pin_busy = FLXOS_PIN_BUSY;
			cfg.memory_width = FLXOS_DISPLAY_WIDTH;
			cfg.memory_height = FLXOS_DISPLAY_HEIGHT;
			cfg.panel_width = FLXOS_DISPLAY_WIDTH;
			cfg.panel_height = FLXOS_DISPLAY_HEIGHT;
			cfg.offset_x = FLXOS_PANEL_OFFSET_X;
			cfg.offset_y = FLXOS_PANEL_OFFSET_Y;
			cfg.offset_rotation = FLXOS_PANEL_OFFSET_ROTATION;
			cfg.dummy_read_pixel = FLXOS_DUMMY_READ_PIXEL;
			cfg.dummy_read_bits = FLXOS_DUMMY_READ_BITS;
			cfg.readable = FLXOS_PANEL_READABLE;
			cfg.invert = FLXOS_DISPLAY_INVERT;
			cfg.rgb_order = FLXOS_DISPLAY_RGB_ORDER;
			cfg.dlen_16bit = FLXOS_PANEL_DLEN_16BIT;
			cfg.bus_shared = FLXOS_BUS_SHARED;
			_panel_instance.config(cfg);
		}

		// ====================================================================
		// Backlight Configuration
		// ====================================================================
		{
			auto cfg = _light_instance.config();
			cfg.pin_bl = FLXOS_PIN_BCKL;
			cfg.invert = FLXOS_BCKL_INVERT;
			cfg.freq = FLXOS_BCKL_FREQ;
			cfg.pwm_channel = FLXOS_BCKL_PWM_CHANNEL;
			_light_instance.config(cfg);
			_panel_instance.setLight(&_light_instance);
		}

#if FLXOS_TOUCH_ENABLED
		// ====================================================================
		// Touch Configuration (XPT2046)
		// ====================================================================
		{
			auto cfg = _touch_instance.config();
			cfg.x_min = FLXOS_TOUCH_X_MIN;
			cfg.x_max = FLXOS_TOUCH_X_MAX;
			cfg.y_min = FLXOS_TOUCH_Y_MIN;
			cfg.y_max = FLXOS_TOUCH_Y_MAX;
			cfg.pin_int = FLXOS_PIN_TOUCH_INT;
			cfg.bus_shared = FLXOS_TOUCH_BUS_SHARED;
			cfg.offset_rotation = FLXOS_TOUCH_OFFSET_ROTATION;

			// Use dedicated touch SPI pins/host when requested, otherwise reuse display SPI.
#if FLXOS_TOUCH_SPI_SEPARATE_PINS
			cfg.spi_host = FLXOS_TOUCH_SPI_HOST;
			cfg.pin_sclk = FLXOS_PIN_TOUCH_SCLK;
			cfg.pin_mosi = FLXOS_PIN_TOUCH_MOSI;
			cfg.pin_miso = FLXOS_PIN_TOUCH_MISO;
#else
			cfg.spi_host = FLXOS_SPI_HOST;
			cfg.pin_sclk = FLXOS_PIN_SCLK;
			cfg.pin_mosi = FLXOS_PIN_MOSI;
			cfg.pin_miso = FLXOS_PIN_MISO;
#endif
			cfg.freq = FLXOS_TOUCH_SPI_FREQ;
			cfg.pin_cs = FLXOS_PIN_TOUCH_CS;

			_touch_instance.config(cfg);
			_panel_instance.setTouch(&_touch_instance);
		}
#endif

		setPanel(&_panel_instance);
	}
};

typedef struct {
	LGFX* tft;
} lv_lovyan_gfx_driver_data_t;
