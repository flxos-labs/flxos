#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <sdkconfig.h>

// ============================================================================
// Display Panel Type Selection (from Kconfig)
// ============================================================================
#if defined(CONFIG_FLXOS_DISPLAY_ILI9163)
#define LGFX_PANEL_TYPE lgfx::Panel_ILI9163
#elif defined(CONFIG_FLXOS_DISPLAY_ILI9225)
#define LGFX_PANEL_TYPE lgfx::Panel_ILI9225
#elif defined(CONFIG_FLXOS_DISPLAY_ILI9341)
#define LGFX_PANEL_TYPE lgfx::Panel_ILI9341
#elif defined(CONFIG_FLXOS_DISPLAY_ILI9342)
#define LGFX_PANEL_TYPE lgfx::Panel_ILI9342
#elif defined(CONFIG_FLXOS_DISPLAY_ILI9481)
#define LGFX_PANEL_TYPE lgfx::Panel_ILI9481
#elif defined(CONFIG_FLXOS_DISPLAY_ILI9486)
#define LGFX_PANEL_TYPE lgfx::Panel_ILI9486
#elif defined(CONFIG_FLXOS_DISPLAY_ILI9488)
#define LGFX_PANEL_TYPE lgfx::Panel_ILI9488
#elif defined(CONFIG_FLXOS_DISPLAY_ST7735)
#define LGFX_PANEL_TYPE lgfx::Panel_ST7735
#elif defined(CONFIG_FLXOS_DISPLAY_ST7789)
#define LGFX_PANEL_TYPE lgfx::Panel_ST7789
#elif defined(CONFIG_FLXOS_DISPLAY_ST7796)
#define LGFX_PANEL_TYPE lgfx::Panel_ST7796
#elif defined(CONFIG_FLXOS_DISPLAY_GC9A01)
#define LGFX_PANEL_TYPE lgfx::Panel_GC9A01
#elif defined(CONFIG_FLXOS_DISPLAY_SSD1306)
#define LGFX_PANEL_TYPE lgfx::Panel_SSD1306
#elif defined(CONFIG_FLXOS_DISPLAY_SSD1327)
#define LGFX_PANEL_TYPE lgfx::Panel_SSD1327
#elif defined(CONFIG_FLXOS_DISPLAY_SSD1331)
#define LGFX_PANEL_TYPE lgfx::Panel_SSD1331
#elif defined(CONFIG_FLXOS_DISPLAY_SSD1351)
#define LGFX_PANEL_TYPE lgfx::Panel_SSD1351
#elif defined(CONFIG_FLXOS_DISPLAY_SSD1963)
#define LGFX_PANEL_TYPE lgfx::Panel_SSD1963
#elif defined(CONFIG_FLXOS_DISPLAY_NT35510)
#define LGFX_PANEL_TYPE lgfx::Panel_NT35510
#elif defined(CONFIG_FLXOS_DISPLAY_RM68120)
#define LGFX_PANEL_TYPE lgfx::Panel_RM68120
#elif defined(CONFIG_FLXOS_DISPLAY_RA8875)
#define LGFX_PANEL_TYPE lgfx::Panel_RA8875
#endif

// ============================================================================
// Touch Controller Type Selection (from Kconfig)
// ============================================================================
#if defined(CONFIG_FLXOS_TOUCH_ENABLED)
#if defined(CONFIG_FLXOS_TOUCH_XPT2046)
#define LGFX_TOUCH_TYPE lgfx::Touch_XPT2046
#define LGFX_TOUCH_IS_SPI 1
#elif defined(CONFIG_FLXOS_TOUCH_FT5x06)
#define LGFX_TOUCH_TYPE lgfx::Touch_FT5x06
#define LGFX_TOUCH_IS_I2C 1
#elif defined(CONFIG_FLXOS_TOUCH_GT911)
#define LGFX_TOUCH_TYPE lgfx::Touch_GT911
#define LGFX_TOUCH_IS_I2C 1
#elif defined(CONFIG_FLXOS_TOUCH_CSTxxx)
#define LGFX_TOUCH_TYPE lgfx::Touch_CST816S
#define LGFX_TOUCH_IS_I2C 1
#elif defined(CONFIG_FLXOS_TOUCH_CHSC6x)
#define LGFX_TOUCH_TYPE lgfx::Touch_CHSC6x
#define LGFX_TOUCH_IS_I2C 1
#elif defined(CONFIG_FLXOS_TOUCH_GSLx680)
#define LGFX_TOUCH_TYPE lgfx::Touch_GSLx680
#define LGFX_TOUCH_IS_I2C 1
#elif defined(CONFIG_FLXOS_TOUCH_NS2009)
#define LGFX_TOUCH_TYPE lgfx::Touch_NS2009
#define LGFX_TOUCH_IS_I2C 1
#elif defined(CONFIG_FLXOS_TOUCH_STMPE610)
#define LGFX_TOUCH_TYPE lgfx::Touch_STMPE610
#define LGFX_TOUCH_IS_I2C 1
#endif
#endif

// ============================================================================
// SPI Host Selection
// ============================================================================
#if CONFIG_FLXOS_SPI_HOST == 1
#define LGFX_SPI_HOST SPI2_HOST
#else
#define LGFX_SPI_HOST SPI3_HOST
#endif

// ============================================================================
// DMA Channel Selection
// ============================================================================
#if CONFIG_FLXOS_SPI_DMA_CHANNEL == 0
#define LGFX_DMA_CHANNEL SPI_DMA_CH_AUTO
#elif CONFIG_FLXOS_SPI_DMA_CHANNEL == 1
#define LGFX_DMA_CHANNEL SPI_DMA_CH1
#else
#define LGFX_DMA_CHANNEL SPI_DMA_CH2
#endif

// ============================================================================
// LovyanGFX Configuration Class
// ============================================================================
class LGFX : public lgfx::LGFX_Device {
	LGFX_PANEL_TYPE _panel_instance;
	lgfx::Bus_SPI _bus_instance;
	lgfx::Light_PWM _light_instance;
#if defined(CONFIG_FLXOS_TOUCH_ENABLED)
	LGFX_TOUCH_TYPE _touch_instance;
#endif

public:

	LGFX(void) {
		// ====================================================================
		// SPI Bus Configuration
		// ====================================================================
		{
			auto cfg = _bus_instance.config();
			cfg.spi_host = LGFX_SPI_HOST;
			cfg.spi_mode = CONFIG_FLXOS_SPI_MODE;
			cfg.freq_write = CONFIG_FLXOS_SPI_FREQ_WRITE;
			cfg.freq_read = CONFIG_FLXOS_SPI_FREQ_READ;
#if defined(CONFIG_FLXOS_SPI_3WIRE)
			cfg.spi_3wire = true;
#else
			cfg.spi_3wire = false;
#endif
			cfg.use_lock = true;
			cfg.dma_channel = LGFX_DMA_CHANNEL;
			cfg.pin_sclk = CONFIG_FLXOS_PIN_SCLK;
			cfg.pin_mosi = CONFIG_FLXOS_PIN_MOSI;
			cfg.pin_miso = CONFIG_FLXOS_PIN_MISO;
			cfg.pin_dc = CONFIG_FLXOS_PIN_DC;
			_bus_instance.config(cfg);
			_panel_instance.setBus(&_bus_instance);
		}

		// ====================================================================
		// Panel Configuration
		// ====================================================================
		{
			auto cfg = _panel_instance.config();
			cfg.pin_cs = CONFIG_FLXOS_PIN_CS;
			cfg.pin_rst = CONFIG_FLXOS_PIN_RST;
			cfg.pin_busy = CONFIG_FLXOS_PIN_BUSY;
			cfg.memory_width = CONFIG_FLXOS_DISPLAY_WIDTH;
			cfg.memory_height = CONFIG_FLXOS_DISPLAY_HEIGHT;
			cfg.panel_width = CONFIG_FLXOS_DISPLAY_WIDTH;
			cfg.panel_height = CONFIG_FLXOS_DISPLAY_HEIGHT;
			cfg.offset_x = CONFIG_FLXOS_PANEL_OFFSET_X;
			cfg.offset_y = CONFIG_FLXOS_PANEL_OFFSET_Y;
			cfg.offset_rotation = CONFIG_FLXOS_PANEL_OFFSET_ROTATION;
			cfg.dummy_read_pixel = CONFIG_FLXOS_DUMMY_READ_PIXEL;
			cfg.dummy_read_bits = CONFIG_FLXOS_DUMMY_READ_BITS;
#if defined(CONFIG_FLXOS_PANEL_READABLE)
			cfg.readable = true;
#else
			cfg.readable = false;
#endif
#if defined(CONFIG_FLXOS_DISPLAY_INVERT)
			cfg.invert = true;
#else
			cfg.invert = false;
#endif
#if defined(CONFIG_FLXOS_DISPLAY_RGB_ORDER)
			cfg.rgb_order = true;
#else
			cfg.rgb_order = false;
#endif
#if defined(CONFIG_FLXOS_PANEL_DLEN_16BIT)
			cfg.dlen_16bit = true;
#else
			cfg.dlen_16bit = false;
#endif
#if defined(CONFIG_FLXOS_BUS_SHARED)
			cfg.bus_shared = true;
#else
			cfg.bus_shared = false;
#endif
			_panel_instance.config(cfg);
		}

		// ====================================================================
		// Backlight Configuration
		// ====================================================================
		{
			auto cfg = _light_instance.config();
			cfg.pin_bl = CONFIG_FLXOS_PIN_BCKL;
#if defined(CONFIG_FLXOS_BCKL_INVERT)
			cfg.invert = true;
#else
			cfg.invert = false;
#endif
			cfg.freq = CONFIG_FLXOS_BCKL_FREQ;
			cfg.pwm_channel = CONFIG_FLXOS_BCKL_PWM_CHANNEL;
			_light_instance.config(cfg);
			_panel_instance.setLight(&_light_instance);
		}

		// ====================================================================
		// Touch Configuration
		// ====================================================================
#if defined(CONFIG_FLXOS_TOUCH_ENABLED)
		{
			auto cfg = _touch_instance.config();
			// Use calibration values if provided, otherwise use display dimensions
#if CONFIG_FLXOS_TOUCH_X_MAX == 0
			cfg.x_min = CONFIG_FLXOS_TOUCH_X_MIN;
			cfg.x_max = CONFIG_FLXOS_DISPLAY_WIDTH - 1;
#else
			cfg.x_min = CONFIG_FLXOS_TOUCH_X_MIN;
			cfg.x_max = CONFIG_FLXOS_TOUCH_X_MAX;
#endif
#if CONFIG_FLXOS_TOUCH_Y_MAX == 0
			cfg.y_min = CONFIG_FLXOS_TOUCH_Y_MIN;
			cfg.y_max = CONFIG_FLXOS_DISPLAY_HEIGHT - 1;
#else
			cfg.y_min = CONFIG_FLXOS_TOUCH_Y_MIN;
			cfg.y_max = CONFIG_FLXOS_TOUCH_Y_MAX;
#endif
			cfg.pin_int = CONFIG_FLXOS_PIN_TOUCH_INT;
#if defined(CONFIG_FLXOS_TOUCH_BUS_SHARED)
			cfg.bus_shared = true;
#else
			cfg.bus_shared = false;
#endif
			cfg.offset_rotation = CONFIG_FLXOS_TOUCH_OFFSET_ROTATION;

#if defined(LGFX_TOUCH_IS_SPI)
			// SPI Touch Controller (XPT2046)
			cfg.spi_host = LGFX_SPI_HOST;
			cfg.freq = CONFIG_FLXOS_TOUCH_SPI_FREQ;
			cfg.pin_sclk = CONFIG_FLXOS_PIN_SCLK;
			cfg.pin_mosi = CONFIG_FLXOS_PIN_MOSI;
			cfg.pin_miso = CONFIG_FLXOS_PIN_MISO;
			cfg.pin_cs = CONFIG_FLXOS_PIN_TOUCH_CS;
#elif defined(LGFX_TOUCH_IS_I2C)
			// I2C Touch Controller
			cfg.i2c_port = CONFIG_FLXOS_TOUCH_I2C_PORT;
			cfg.freq = CONFIG_FLXOS_TOUCH_I2C_FREQ;
			cfg.pin_scl = CONFIG_FLXOS_PIN_TOUCH_SCL;
			cfg.pin_sda = CONFIG_FLXOS_PIN_TOUCH_SDA;
#endif
			_touch_instance.config(cfg);
			_panel_instance.setTouch(&_touch_instance);
		}
#endif

		setPanel(&_panel_instance);
	}
};
