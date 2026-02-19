#pragma once

#define LGFX_USE_V1

#include <LovyanGFX.hpp>
#include <sdkconfig.h>

// ============================================================================
// LovyanGFX Configuration Class (Hardcoded for esp32s3-ili9341-xpt)
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
#pragma message "Building LGFX User Header - LGFX Class Defined (Hardcoded)"

	LGFX(void) {
		// ====================================================================
		// Bus Configuration (SPI)
		// ====================================================================
		{
			auto cfg = _bus_instance.config();
			cfg.spi_host = SPI2_HOST;
			cfg.spi_mode = 0;
			cfg.freq_write = 40000000;
			cfg.freq_read = 16000000;
			cfg.spi_3wire = false;
			cfg.use_lock = true;
			cfg.dma_channel = SPI_DMA_CH_AUTO;
			cfg.pin_sclk = 12;
			cfg.pin_mosi = 11;
			cfg.pin_miso = 13;
			cfg.pin_dc = 9;
			_bus_instance.config(cfg);
			_panel_instance.setBus(&_bus_instance);
		}

		// ====================================================================
		// Panel Configuration (ILI9341)
		// ====================================================================
		{
			auto cfg = _panel_instance.config();
			cfg.pin_cs = 10;
			cfg.pin_rst = 14;
			cfg.pin_busy = -1;
			cfg.memory_width = 240;
			cfg.memory_height = 320;
			cfg.panel_width = 240;
			cfg.panel_height = 320;
			cfg.offset_x = 0;
			cfg.offset_y = 0;
			cfg.offset_rotation = 0;
			cfg.dummy_read_pixel = 8;
			cfg.dummy_read_bits = 1;
			cfg.readable = true;
			cfg.invert = false;
			cfg.rgb_order = false;
			cfg.dlen_16bit = false;
			cfg.bus_shared = true;
			_panel_instance.config(cfg);
		}

		// ====================================================================
		// Backlight Configuration
		// ====================================================================
		{
			auto cfg = _light_instance.config();
			cfg.pin_bl = 7;
			cfg.invert = false;
			cfg.freq = 20000;
			cfg.pwm_channel = 0;
			_light_instance.config(cfg);
			_panel_instance.setLight(&_light_instance);
		}

		// ====================================================================
		// Touch Configuration (XPT2046)
		// ====================================================================
		{
			auto cfg = _touch_instance.config();
			cfg.x_min = 3800;
			cfg.x_max = 200;
			cfg.y_min = 200;
			cfg.y_max = 3800;
			cfg.pin_int = 6;
			cfg.bus_shared = true;
			cfg.offset_rotation = 0;

			// Shared SPI bus — touch reuses display SPI pins
			cfg.spi_host = SPI2_HOST;
			cfg.freq = 1000000;
			cfg.pin_sclk = 12;
			cfg.pin_mosi = 11;
			cfg.pin_miso = 13;
			cfg.pin_cs = 5;

			_touch_instance.config(cfg);
			_panel_instance.setTouch(&_touch_instance);
		}

		setPanel(&_panel_instance);
	}
};

typedef struct {
	LGFX* tft;
} lv_lovyan_gfx_driver_data_t;
