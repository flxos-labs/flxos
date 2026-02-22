#pragma once

#define LGFX_USE_V1

#include <Config.hpp>
#include <LovyanGFX.hpp>

// Compile-time validation — this header should only be included for display-enabled profiles
static_assert(flx::config::display.enabled, "LGFX header included but display is disabled in profile.yaml");

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

	LGFX(void) {
		// ====================================================================
		// Bus Configuration (SPI)
		// ====================================================================
		{
			auto cfg = _bus_instance.config();
			cfg.spi_host = flx::config::display.spi.host;
			cfg.spi_mode = flx::config::display.spi.mode;
			cfg.freq_write = flx::config::display.spi.freqWrite;
			cfg.freq_read = flx::config::display.spi.freqRead;
			cfg.spi_3wire = flx::config::display.spi.threeWire;
			cfg.use_lock = true;
			cfg.dma_channel =
				(flx::config::display.spi.dmaChannel == 0) ? SPI_DMA_CH_AUTO : (flx::config::display.spi.dmaChannel == 1) ? 1
				: (flx::config::display.spi.dmaChannel == 2)															  ? 2
																														  : SPI_DMA_CH_AUTO;
			cfg.pin_sclk = flx::config::display.pins.sclk;
			cfg.pin_mosi = flx::config::display.pins.mosi;
			cfg.pin_miso = flx::config::display.pins.miso;
			cfg.pin_dc = flx::config::display.pins.dc;
			_bus_instance.config(cfg);
			_panel_instance.setBus(&_bus_instance);
		}

		// ====================================================================
		// Panel Configuration (ILI9341)
		// ====================================================================
		{
			auto cfg = _panel_instance.config();
			cfg.pin_cs = flx::config::display.pins.cs;
			cfg.pin_rst = flx::config::display.pins.rst;
			cfg.pin_busy = flx::config::display.pins.busy;
			cfg.memory_width = flx::config::display.width;
			cfg.memory_height = flx::config::display.height;
			cfg.panel_width = flx::config::display.width;
			cfg.panel_height = flx::config::display.height;
			cfg.offset_x = flx::config::display.panel.offsetX;
			cfg.offset_y = flx::config::display.panel.offsetY;
			cfg.offset_rotation = flx::config::display.panel.offsetRotation;
			cfg.dummy_read_pixel = flx::config::display.panel.dummyReadPixel;
			cfg.dummy_read_bits = flx::config::display.panel.dummyReadBits;
			cfg.readable = flx::config::display.panel.readable;
			cfg.invert = flx::config::display.panel.invert;
			cfg.rgb_order = flx::config::display.panel.rgbOrder;
			cfg.dlen_16bit = flx::config::display.panel.dlen16bit;
			cfg.bus_shared = flx::config::display.panel.busShared;
			_panel_instance.config(cfg);
		}

		// ====================================================================
		// Backlight Configuration
		// ====================================================================
		{
			auto cfg = _light_instance.config();
			cfg.pin_bl = flx::config::display.pins.bckl;
			cfg.invert = flx::config::display.backlight.invert;
			cfg.freq = flx::config::display.backlight.freq;
			cfg.pwm_channel = flx::config::display.backlight.pwmChannel;
			_light_instance.config(cfg);
			_panel_instance.setLight(&_light_instance);
		}

		if constexpr (flx::config::touch.enabled) {
			// ====================================================================
			// Touch Configuration (XPT2046)
			// ====================================================================
			{
				auto cfg = _touch_instance.config();
				cfg.x_min = flx::config::touch.calibration.xMin;
				cfg.x_max = flx::config::touch.calibration.xMax;
				cfg.y_min = flx::config::touch.calibration.yMin;
				cfg.y_max = flx::config::touch.calibration.yMax;
				cfg.pin_int = flx::config::touch.pins.interrupt;
				cfg.bus_shared = flx::config::touch.spi.busShared;
				cfg.offset_rotation = flx::config::touch.calibration.offsetRotation;

				if constexpr (flx::config::touch.spi.separatePins) {
					// Dedicated touch SPI pins/host
					cfg.spi_host = flx::config::touch.spi.host;
					cfg.pin_sclk = flx::config::touch.pins.sclk;
					cfg.pin_mosi = flx::config::touch.pins.mosi;
					cfg.pin_miso = flx::config::touch.pins.miso;
				} else {
					// Reuse display SPI
					cfg.spi_host = flx::config::display.spi.host;
					cfg.pin_sclk = flx::config::display.pins.sclk;
					cfg.pin_mosi = flx::config::display.pins.mosi;
					cfg.pin_miso = flx::config::display.pins.miso;
				}
				cfg.freq = flx::config::touch.spi.freq;
				cfg.pin_cs = flx::config::touch.pins.cs;

				_touch_instance.config(cfg);
				_panel_instance.setTouch(&_touch_instance);
			}
		}

		setPanel(&_panel_instance);
	}
};

typedef struct {
	LGFX* tft;
} lv_lovyan_gfx_driver_data_t;
