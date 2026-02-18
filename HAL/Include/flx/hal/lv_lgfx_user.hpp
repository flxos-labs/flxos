#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <sdkconfig.h>
#include "Config.hpp"

// Compatibility: Map Profile (FLXOS_*) macros to Kconfig (CONFIG_FLXOS_*) macros
// if the Kconfig ones are missing (because the profile hides them in menuconfig).

#if defined(FLXOS_DISPLAY_ILI9341) && !defined(CONFIG_FLXOS_DISPLAY_ILI9341)
    #define CONFIG_FLXOS_DISPLAY_ILI9341 1
#endif
#if defined(FLXOS_BUS_SPI) && !defined(CONFIG_FLXOS_BUS_SPI)
    #define CONFIG_FLXOS_BUS_SPI 1
#endif
#if defined(FLXOS_TOUCH_XPT2046) && !defined(CONFIG_FLXOS_TOUCH_XPT2046)
    #define CONFIG_FLXOS_TOUCH_XPT2046 1
#endif
#if defined(FLXOS_TOUCH_ENABLED) && !defined(CONFIG_FLXOS_TOUCH_ENABLED)
    #define CONFIG_FLXOS_TOUCH_ENABLED 1
#endif
#if defined(FLXOS_SPI_HOST) && !defined(CONFIG_FLXOS_SPI_HOST)
    // Map SPI2_HOST etc to integer expected by logic?
    // The logic expects CONFIG_FLXOS_SPI_HOST == 1 (SPI2) or 2 (SPI3).
    // Config.hpp defines FLXOS_SPI_HOST as SPI2_HOST (which is likely an enum/macro from IDF).
    // We need to check the value.
    #if FLXOS_SPI_HOST == SPI2_HOST
        #define CONFIG_FLXOS_SPI_HOST 1
    #elif FLXOS_SPI_HOST == SPI3_HOST
        #define CONFIG_FLXOS_SPI_HOST 2
    #endif
#endif
#if defined(FLXOS_SPI_DMA_CHANNEL) && !defined(CONFIG_FLXOS_SPI_DMA_CHANNEL)
     #define CONFIG_FLXOS_SPI_DMA_CHANNEL FLXOS_SPI_DMA_CHANNEL
#endif
// Map other values used in #if checks
#if defined(FLXOS_BCKL_INVERT) && !defined(CONFIG_FLXOS_BCKL_INVERT)
    #if FLXOS_BCKL_INVERT
        #define CONFIG_FLXOS_BCKL_INVERT 1
    #endif
#endif
#if defined(FLXOS_DISPLAY_INVERT) && !defined(CONFIG_FLXOS_DISPLAY_INVERT)
    #if FLXOS_DISPLAY_INVERT
        #define CONFIG_FLXOS_DISPLAY_INVERT 1
    #endif
#endif
#if defined(FLXOS_DISPLAY_RGB_ORDER) && !defined(CONFIG_FLXOS_DISPLAY_RGB_ORDER)
    #if FLXOS_DISPLAY_RGB_ORDER
        #define CONFIG_FLXOS_DISPLAY_RGB_ORDER 1
    #endif
#endif
#if defined(FLXOS_PANEL_READABLE) && !defined(CONFIG_FLXOS_PANEL_READABLE)
    #if FLXOS_PANEL_READABLE
        #define CONFIG_FLXOS_PANEL_READABLE 1
    #endif
#endif
#if defined(FLXOS_PANEL_DLEN_16BIT) && !defined(CONFIG_FLXOS_PANEL_DLEN_16BIT)
    #if FLXOS_PANEL_DLEN_16BIT
        #define CONFIG_FLXOS_PANEL_DLEN_16BIT 1
    #endif
#endif
#if defined(FLXOS_BUS_SHARED) && !defined(CONFIG_FLXOS_BUS_SHARED)
    #if FLXOS_BUS_SHARED
        #define CONFIG_FLXOS_BUS_SHARED 1
    #endif
#endif
#if defined(FLXOS_TOUCH_BUS_SHARED) && !defined(CONFIG_FLXOS_TOUCH_BUS_SHARED)
    #if FLXOS_TOUCH_BUS_SHARED
        #define CONFIG_FLXOS_TOUCH_BUS_SHARED 1
    #endif
#endif

// Map Pins and Values (The code uses CONFIG_FLXOS_PIN_* inside the class)
// We need to define them if they match FLXOS_PIN_*
#ifndef CONFIG_FLXOS_PIN_CS
    #define CONFIG_FLXOS_PIN_CS FLXOS_PIN_CS
#endif
#ifndef CONFIG_FLXOS_PIN_RST
    #define CONFIG_FLXOS_PIN_RST FLXOS_PIN_RST
#endif
#ifndef CONFIG_FLXOS_PIN_BUSY
    #define CONFIG_FLXOS_PIN_BUSY FLXOS_PIN_BUSY
#endif
#ifndef CONFIG_FLXOS_DISPLAY_WIDTH
    #define CONFIG_FLXOS_DISPLAY_WIDTH FLXOS_DISPLAY_WIDTH
#endif
#ifndef CONFIG_FLXOS_DISPLAY_HEIGHT
    #define CONFIG_FLXOS_DISPLAY_HEIGHT FLXOS_DISPLAY_HEIGHT
#endif
#ifndef CONFIG_FLXOS_PANEL_OFFSET_X
    #define CONFIG_FLXOS_PANEL_OFFSET_X FLXOS_PANEL_OFFSET_X
#endif
#ifndef CONFIG_FLXOS_PANEL_OFFSET_Y
    #define CONFIG_FLXOS_PANEL_OFFSET_Y FLXOS_PANEL_OFFSET_Y
#endif
#ifndef CONFIG_FLXOS_PANEL_OFFSET_ROTATION
    #define CONFIG_FLXOS_PANEL_OFFSET_ROTATION FLXOS_PANEL_OFFSET_ROTATION
#endif
#ifndef CONFIG_FLXOS_DUMMY_READ_PIXEL
    #define CONFIG_FLXOS_DUMMY_READ_PIXEL FLXOS_DUMMY_READ_PIXEL
#endif
#ifndef CONFIG_FLXOS_DUMMY_READ_BITS
    #define CONFIG_FLXOS_DUMMY_READ_BITS FLXOS_DUMMY_READ_BITS
#endif
#ifndef CONFIG_FLXOS_SPI_MODE
    #define CONFIG_FLXOS_SPI_MODE FLXOS_SPI_MODE
#endif
#ifndef CONFIG_FLXOS_SPI_FREQ_WRITE
    #define CONFIG_FLXOS_SPI_FREQ_WRITE FLXOS_SPI_FREQ_WRITE
#endif
#ifndef CONFIG_FLXOS_SPI_FREQ_READ
    #define CONFIG_FLXOS_SPI_FREQ_READ FLXOS_SPI_FREQ_READ
#endif
#ifndef CONFIG_FLXOS_PIN_SCLK
    #define CONFIG_FLXOS_PIN_SCLK FLXOS_PIN_SCLK
#endif
#ifndef CONFIG_FLXOS_PIN_MOSI
    #define CONFIG_FLXOS_PIN_MOSI FLXOS_PIN_MOSI
#endif
#ifndef CONFIG_FLXOS_PIN_MISO
    #define CONFIG_FLXOS_PIN_MISO FLXOS_PIN_MISO
#endif
#ifndef CONFIG_FLXOS_PIN_DC
    #define CONFIG_FLXOS_PIN_DC FLXOS_PIN_DC
#endif
#ifndef CONFIG_FLXOS_PIN_BCKL
    #define CONFIG_FLXOS_PIN_BCKL FLXOS_PIN_BCKL
#endif
#ifndef CONFIG_FLXOS_BCKL_FREQ
    #define CONFIG_FLXOS_BCKL_FREQ FLXOS_BCKL_FREQ
#endif
#ifndef CONFIG_FLXOS_BCKL_PWM_CHANNEL
    #define CONFIG_FLXOS_BCKL_PWM_CHANNEL FLXOS_BCKL_PWM_CHANNEL
#endif
#ifndef CONFIG_FLXOS_TOUCH_X_MIN
    #define CONFIG_FLXOS_TOUCH_X_MIN FLXOS_TOUCH_X_MIN
#endif
#ifndef CONFIG_FLXOS_TOUCH_X_MAX
    #define CONFIG_FLXOS_TOUCH_X_MAX FLXOS_TOUCH_X_MAX
#endif
#ifndef CONFIG_FLXOS_TOUCH_Y_MIN
    #define CONFIG_FLXOS_TOUCH_Y_MIN FLXOS_TOUCH_Y_MIN
#endif
#ifndef CONFIG_FLXOS_TOUCH_Y_MAX
    #define CONFIG_FLXOS_TOUCH_Y_MAX FLXOS_TOUCH_Y_MAX
#endif
#ifndef CONFIG_FLXOS_PIN_TOUCH_INT
    #define CONFIG_FLXOS_PIN_TOUCH_INT FLXOS_PIN_TOUCH_INT
#endif
#ifndef CONFIG_FLXOS_TOUCH_OFFSET_ROTATION
    #define CONFIG_FLXOS_TOUCH_OFFSET_ROTATION FLXOS_TOUCH_OFFSET_ROTATION
#endif
#ifndef CONFIG_FLXOS_PIN_TOUCH_CS
    #define CONFIG_FLXOS_PIN_TOUCH_CS FLXOS_PIN_TOUCH_CS
#endif
#ifndef CONFIG_FLXOS_TOUCH_SPI_FREQ
    #define CONFIG_FLXOS_TOUCH_SPI_FREQ FLXOS_TOUCH_SPI_FREQ
#endif



// ============================================================================
// Display Panel Type Selection
// ============================================================================
#if defined(CONFIG_FLXOS_DISPLAY_AMOLED)
#define LGFX_PANEL_TYPE lgfx::Panel_AMOLED
#elif defined(CONFIG_FLXOS_DISPLAY_CO5300)
#define LGFX_PANEL_TYPE lgfx::Panel_CO5300
#elif defined(CONFIG_FLXOS_DISPLAY_GC9A01)
#define LGFX_PANEL_TYPE lgfx::Panel_GC9A01
#elif defined(CONFIG_FLXOS_DISPLAY_GDEW0154D67)
#define LGFX_PANEL_TYPE lgfx::Panel_GDEW0154D67
#elif defined(CONFIG_FLXOS_DISPLAY_GDEW0154M09)
#define LGFX_PANEL_TYPE lgfx::Panel_GDEW0154M09
#elif defined(CONFIG_FLXOS_DISPLAY_HUB75)
#define LGFX_PANEL_TYPE lgfx::Panel_HUB75
#elif defined(CONFIG_FLXOS_DISPLAY_ILI9163)
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
#elif defined(CONFIG_FLXOS_DISPLAY_ILI9806)
#define LGFX_PANEL_TYPE lgfx::Panel_ILI9806
#elif defined(CONFIG_FLXOS_DISPLAY_IT8951)
#define LGFX_PANEL_TYPE lgfx::Panel_IT8951
#elif defined(CONFIG_FLXOS_DISPLAY_M5HDMI)
#define LGFX_PANEL_TYPE lgfx::Panel_M5HDMI
#elif defined(CONFIG_FLXOS_DISPLAY_M5UnitGLASS)
#define LGFX_PANEL_TYPE lgfx::Panel_M5UnitGLASS
#elif defined(CONFIG_FLXOS_DISPLAY_M5UnitLCD)
#define LGFX_PANEL_TYPE lgfx::Panel_M5UnitLCD
#elif defined(CONFIG_FLXOS_DISPLAY_NT35510)
#define LGFX_PANEL_TYPE lgfx::Panel_NT35510
#elif defined(CONFIG_FLXOS_DISPLAY_NV3041A)
#define LGFX_PANEL_TYPE lgfx::Panel_NV3041A
#elif defined(CONFIG_FLXOS_DISPLAY_R61529)
#define LGFX_PANEL_TYPE lgfx::Panel_R61529
#elif defined(CONFIG_FLXOS_DISPLAY_RA8875)
#define LGFX_PANEL_TYPE lgfx::Panel_RA8875
#elif defined(CONFIG_FLXOS_DISPLAY_RM67162)
#define LGFX_PANEL_TYPE lgfx::Panel_RM67162
#elif defined(CONFIG_FLXOS_DISPLAY_RM68120)
#define LGFX_PANEL_TYPE lgfx::Panel_RM68120
#elif defined(CONFIG_FLXOS_DISPLAY_RM690B0)
#define LGFX_PANEL_TYPE lgfx::Panel_RM690B0
#elif defined(CONFIG_FLXOS_DISPLAY_S6D04K1)
#define LGFX_PANEL_TYPE lgfx::Panel_S6D04K1
#elif defined(CONFIG_FLXOS_DISPLAY_SH8601Z)
#define LGFX_PANEL_TYPE lgfx::Panel_SH8601Z
#elif defined(CONFIG_FLXOS_DISPLAY_SharpLCD)
#define LGFX_PANEL_TYPE lgfx::Panel_SharpLCD
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
#elif defined(CONFIG_FLXOS_DISPLAY_ST7735)
#define LGFX_PANEL_TYPE lgfx::Panel_ST7735
#elif defined(CONFIG_FLXOS_DISPLAY_ST7789)
#define LGFX_PANEL_TYPE lgfx::Panel_ST7789
#elif defined(CONFIG_FLXOS_DISPLAY_ST7789P3)
#define LGFX_PANEL_TYPE lgfx::Panel_ST7789P3
#elif defined(CONFIG_FLXOS_DISPLAY_ST7796)
#define LGFX_PANEL_TYPE lgfx::Panel_ST7796
#elif defined(CONFIG_FLXOS_DISPLAY_ST77961)
#define LGFX_PANEL_TYPE lgfx::Panel_ST77961
#endif

// ============================================================================
// Touch Controller Type Selection
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
#elif defined(CONFIG_FLXOS_TOUCH_RA8875)
#define LGFX_TOUCH_TYPE lgfx::Touch_RA8875
#define LGFX_TOUCH_IS_I2C 1
#elif defined(CONFIG_FLXOS_TOUCH_STMPE610)
#define LGFX_TOUCH_TYPE lgfx::Touch_STMPE610
#define LGFX_TOUCH_IS_I2C 1
#elif defined(CONFIG_FLXOS_TOUCH_TT21xxx)
#define LGFX_TOUCH_TYPE lgfx::Touch_TT21xxx
#define LGFX_TOUCH_IS_I2C 1
#endif
#endif

// ============================================================================
// Bus Type Selection
// ============================================================================
#if defined(CONFIG_FLXOS_BUS_SPI)
#define LGFX_BUS_TYPE lgfx::Bus_SPI
#elif defined(CONFIG_FLXOS_BUS_I2C)
#define LGFX_BUS_TYPE lgfx::Bus_I2C
#elif defined(CONFIG_FLXOS_BUS_PARALLEL8)
#define LGFX_BUS_TYPE lgfx::Bus_Parallel8
#elif defined(CONFIG_FLXOS_BUS_PARALLEL16)
#define LGFX_BUS_TYPE lgfx::Bus_Parallel16
#endif

// ============================================================================
// SPI Host Selection (ESP32 Specific)
// ============================================================================
#if defined(CONFIG_FLXOS_SPI_HOST)
#if CONFIG_FLXOS_SPI_HOST == 1
#define LGFX_SPI_HOST SPI2_HOST
#else
#define LGFX_SPI_HOST SPI3_HOST
#endif
#endif

// ============================================================================
// DMA Channel Selection
// ============================================================================
#if defined(CONFIG_FLXOS_SPI_DMA_CHANNEL)
#if CONFIG_FLXOS_SPI_DMA_CHANNEL == 0
#define LGFX_DMA_CHANNEL SPI_DMA_CH_AUTO
#elif CONFIG_FLXOS_SPI_DMA_CHANNEL == 1
#define LGFX_DMA_CHANNEL SPI_DMA_CH1
#else
#define LGFX_DMA_CHANNEL SPI_DMA_CH2
#endif
#endif

// ============================================================================
// LovyanGFX Configuration Class
// ============================================================================
class LGFX : public lgfx::LGFX_Device {
	LGFX_PANEL_TYPE _panel_instance;
	LGFX_BUS_TYPE _bus_instance;
	lgfx::Light_PWM _light_instance;
#if defined(CONFIG_FLXOS_TOUCH_ENABLED)
	LGFX_TOUCH_TYPE _touch_instance;
#endif

public:
    // Explicitly expose base methods to workaround visibility issue in lv_lovyan_gfx.cpp
    int32_t width() const { return lgfx::LGFX_Device::width(); }
    int32_t height() const { return lgfx::LGFX_Device::height(); }
    uint8_t getRotation() const { return lgfx::LGFX_Device::getRotation(); }
    void setRotation(uint8_t r) { lgfx::LGFX_Device::setRotation(r); }
    // Also expose getTouch/getTouchRaw if needed
    
    void test_inheritance() {
        volatile int w = this->width();
        (void)w;
    }
    #pragma message "Building LGFX User Header - LGFX Class Defined"

	LGFX(void) {
		// ====================================================================
		// Bus Configuration
		// ====================================================================
#if defined(CONFIG_FLXOS_BUS_SPI)
		// SPI Bus
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
#elif defined(CONFIG_FLXOS_BUS_I2C)
		// I2C Bus
		{
			auto cfg = _bus_instance.config();
			cfg.i2c_port = CONFIG_FLXOS_I2C_PORT;
			cfg.freq_write = CONFIG_FLXOS_I2C_FREQ;
			cfg.freq_read = CONFIG_FLXOS_I2C_FREQ;
			cfg.pin_sda = CONFIG_FLXOS_PIN_I2C_SDA;
			cfg.pin_scl = CONFIG_FLXOS_PIN_I2C_SCL;
			cfg.i2c_addr = CONFIG_FLXOS_I2C_ADDR;
			_bus_instance.config(cfg);
			_panel_instance.setBus(&_bus_instance);
		}
#elif defined(CONFIG_FLXOS_BUS_PARALLEL8) || defined(CONFIG_FLXOS_BUS_PARALLEL16)
		// Parallel Bus
		{
			auto cfg = _bus_instance.config();
			cfg.freq_write = CONFIG_FLXOS_PARALLEL_FREQ;
			cfg.pin_wr = CONFIG_FLXOS_PIN_WR;
			cfg.pin_rd = CONFIG_FLXOS_PIN_RD;
			cfg.pin_rs = CONFIG_FLXOS_PIN_DC;
			cfg.pin_d0 = CONFIG_FLXOS_PIN_D0;
			cfg.pin_d1 = CONFIG_FLXOS_PIN_D1;
			cfg.pin_d2 = CONFIG_FLXOS_PIN_D2;
			cfg.pin_d3 = CONFIG_FLXOS_PIN_D3;
			cfg.pin_d4 = CONFIG_FLXOS_PIN_D4;
			cfg.pin_d5 = CONFIG_FLXOS_PIN_D5;
			cfg.pin_d6 = CONFIG_FLXOS_PIN_D6;
			cfg.pin_d7 = CONFIG_FLXOS_PIN_D7;
#if defined(CONFIG_FLXOS_BUS_PARALLEL16)
			cfg.pin_d8 = CONFIG_FLXOS_PIN_D8;
			cfg.pin_d9 = CONFIG_FLXOS_PIN_D9;
			cfg.pin_d10 = CONFIG_FLXOS_PIN_D10;
			cfg.pin_d11 = CONFIG_FLXOS_PIN_D11;
			cfg.pin_d12 = CONFIG_FLXOS_PIN_D12;
			cfg.pin_d13 = CONFIG_FLXOS_PIN_D13;
			cfg.pin_d14 = CONFIG_FLXOS_PIN_D14;
			cfg.pin_d15 = CONFIG_FLXOS_PIN_D15;
#endif
			_bus_instance.config(cfg);
			_panel_instance.setBus(&_bus_instance);
		}
#endif

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
			// SPI Touch Controller (e.g., XPT2046)
#if defined(CONFIG_FLXOS_TOUCH_SPI_SEPARATE_PINS)
			// Separate SPI pins for touch (e.g., ESP32-2432S028R / CYD)
			// spi_host = -1 triggers software (bitbang) SPI in LovyanGFX
			cfg.spi_host = CONFIG_FLXOS_TOUCH_SPI_HOST;
			cfg.freq = CONFIG_FLXOS_TOUCH_SPI_FREQ;
			cfg.pin_sclk = CONFIG_FLXOS_PIN_TOUCH_SCLK;
			cfg.pin_mosi = CONFIG_FLXOS_PIN_TOUCH_MOSI;
			cfg.pin_miso = CONFIG_FLXOS_PIN_TOUCH_MISO;
			cfg.pin_cs = CONFIG_FLXOS_PIN_TOUCH_CS;
#else
			// Shared SPI bus — touch reuses display SPI pins
			cfg.spi_host = LGFX_SPI_HOST;
			cfg.freq = CONFIG_FLXOS_TOUCH_SPI_FREQ;
			cfg.pin_sclk = CONFIG_FLXOS_PIN_SCLK;
			cfg.pin_mosi = CONFIG_FLXOS_PIN_MOSI;
			cfg.pin_miso = CONFIG_FLXOS_PIN_MISO;
			cfg.pin_cs = CONFIG_FLXOS_PIN_TOUCH_CS;
#endif

#elif defined(LGFX_TOUCH_IS_I2C)
			// I2C Touch Controller
			cfg.i2c_port = CONFIG_FLXOS_TOUCH_I2C_PORT;
			cfg.freq = CONFIG_FLXOS_TOUCH_I2C_FREQ;

			// NOTE: RA8875 internal touch does not use external I2C pins.
			// Only assign pins if NOT RA8875 to avoid undefined macro errors.
#if !defined(CONFIG_FLXOS_TOUCH_RA8875)
			cfg.pin_scl = CONFIG_FLXOS_PIN_TOUCH_SCL;
			cfg.pin_sda = CONFIG_FLXOS_PIN_TOUCH_SDA;
#endif
#endif
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
