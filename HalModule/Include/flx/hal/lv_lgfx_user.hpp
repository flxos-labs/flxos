#pragma once

#define LGFX_USE_V1

#include <Config.hpp>
#include <LovyanGFX.hpp>

#if defined(CONFIG_IDF_TARGET_ESP32S3) && (defined(FLXOS_DISPLAY_DRIVER_RGB) || defined(FLXOS_DISPLAY_BUS_RGB))
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#elif defined(CONFIG_IDF_TARGET_ESP32P4) && (defined(FLXOS_DISPLAY_DRIVER_ILI9881C) || defined(FLXOS_DISPLAY_BUS_MIPI_DSI))
#include <lgfx/v1/platforms/esp32p4/Bus_DSI.hpp>
#include <lgfx/v1/platforms/esp32p4/Panel_DSI.hpp>
#include <lgfx/v1/platforms/esp32p4/Panel_ILI9881C.hpp>
#endif
#if FLXOS_DISPLAY_DRIVER_EPDIY
#include <lgfx/v1/panel/Panel_EPDiy.hpp>
#endif
#include <cstdint>
#include <limits>
#include <type_traits>

namespace flx::hal::detail {

template<typename T, bool IsEnum = std::is_enum_v<T>>
struct IntegralLike {
	using type = T;
};

template<typename T>
struct IntegralLike<T, true> {
	using type = std::underlying_type_t<T>;
};

template<typename T, typename U>
constexpr T clampCast(U value) {
	using Target = std::remove_cv_t<std::remove_reference_t<T>>;
	using Source = std::remove_cv_t<std::remove_reference_t<U>>;
	using TargetInt = typename IntegralLike<Target>::type;
	using SourceInt = typename IntegralLike<Source>::type;

	static_assert(std::is_integral_v<TargetInt>, "clampCast target must be integral or enum");
	static_assert(std::is_integral_v<SourceInt>, "clampCast source must be integral");
	static_assert(sizeof(SourceInt) <= sizeof(std::intmax_t), "clampCast source width unsupported");

	const auto source = static_cast<std::intmax_t>(static_cast<SourceInt>(value));
	constexpr auto minValue = static_cast<std::intmax_t>(std::numeric_limits<TargetInt>::min());
	constexpr auto maxValue = static_cast<std::intmax_t>(std::numeric_limits<TargetInt>::max());

	if (source < minValue) {
		return static_cast<Target>(static_cast<TargetInt>(minValue));
	}
	if (source > maxValue) {
		return static_cast<Target>(static_cast<TargetInt>(maxValue));
	}
	return static_cast<Target>(static_cast<TargetInt>(source));
}

} // namespace flx::hal::detail

// Compile-time validation — this header should only be included for display-enabled profiles
static_assert(flx::config::display.enabled, "LGFX header included but display is disabled in profile.yaml");

// ============================================================================
// Display Driver Dispatch
// ============================================================================
#if FLXOS_DISPLAY_DRIVER_ILI9163
using LgfxDisplayPanel = lgfx::Panel_ILI9163;
#elif FLXOS_DISPLAY_DRIVER_ILI9225
using LgfxDisplayPanel = lgfx::Panel_ILI9225;
#elif FLXOS_DISPLAY_DRIVER_ILI9341
using LgfxDisplayPanel = lgfx::Panel_ILI9341;
#elif FLXOS_DISPLAY_DRIVER_ILI9342
using LgfxDisplayPanel = lgfx::Panel_ILI9342;
#elif FLXOS_DISPLAY_DRIVER_ILI948X || FLXOS_DISPLAY_DRIVER_ILI9488 || FLXOS_DISPLAY_DRIVER_ILI9481
using LgfxDisplayPanel = lgfx::Panel_ILI948x;
#elif FLXOS_DISPLAY_DRIVER_ILI9806
using LgfxDisplayPanel = lgfx::Panel_ILI9806;
#elif FLXOS_DISPLAY_DRIVER_ST7735
using LgfxDisplayPanel = lgfx::Panel_ST7735;
#elif FLXOS_DISPLAY_DRIVER_ST7789 || FLXOS_DISPLAY_DRIVER_ST7789V
using LgfxDisplayPanel = lgfx::Panel_ST7789;
#elif FLXOS_DISPLAY_DRIVER_ST7789P3
using LgfxDisplayPanel = lgfx::Panel_ST7789P3;
#elif FLXOS_DISPLAY_DRIVER_ST7796
using LgfxDisplayPanel = lgfx::Panel_ST7796;
#elif FLXOS_DISPLAY_DRIVER_ST77961
using LgfxDisplayPanel = lgfx::Panel_ST77961;
#elif FLXOS_DISPLAY_DRIVER_SSD1306
using LgfxDisplayPanel = lgfx::Panel_SSD1306;
#elif FLXOS_DISPLAY_DRIVER_SSD1327
using LgfxDisplayPanel = lgfx::Panel_SSD1327;
#elif FLXOS_DISPLAY_DRIVER_SSD1331
using LgfxDisplayPanel = lgfx::Panel_SSD1331;
#elif FLXOS_DISPLAY_DRIVER_SSD1351
using LgfxDisplayPanel = lgfx::Panel_SSD1351;
#elif FLXOS_DISPLAY_DRIVER_SSD1963
using LgfxDisplayPanel = lgfx::Panel_SSD1963;
#elif FLXOS_DISPLAY_DRIVER_GC9A01
using LgfxDisplayPanel = lgfx::Panel_GC9A01;
#elif FLXOS_DISPLAY_DRIVER_NT35510
using LgfxDisplayPanel = lgfx::Panel_NT35510;
#elif FLXOS_DISPLAY_DRIVER_R61529
using LgfxDisplayPanel = lgfx::Panel_R61529;
#elif FLXOS_DISPLAY_DRIVER_RA8875
using LgfxDisplayPanel = lgfx::Panel_RA8875;
#elif FLXOS_DISPLAY_DRIVER_RM67162
using LgfxDisplayPanel = lgfx::Panel_RM67162;
#elif FLXOS_DISPLAY_DRIVER_RM68120
using LgfxDisplayPanel = lgfx::Panel_RM68120;
#elif FLXOS_DISPLAY_DRIVER_S6D04K1
using LgfxDisplayPanel = lgfx::Panel_S6D04K1;
#elif FLXOS_DISPLAY_DRIVER_NV3041A
using LgfxDisplayPanel = lgfx::Panel_NV3041A;
#elif FLXOS_DISPLAY_DRIVER_SHARPLCD
using LgfxDisplayPanel = lgfx::Panel_SharpLCD;
#elif FLXOS_DISPLAY_DRIVER_CO5300
using LgfxDisplayPanel = lgfx::Panel_CO5300;
#elif FLXOS_DISPLAY_DRIVER_RM690B0
using LgfxDisplayPanel = lgfx::Panel_RM690B0;
#elif FLXOS_DISPLAY_DRIVER_AMOLED
using LgfxDisplayPanel = lgfx::Panel_AMOLED;
#elif FLXOS_DISPLAY_DRIVER_EPDIY
using LgfxDisplayPanel = lgfx::Panel_EPDiy;
#elif FLXOS_DISPLAY_DRIVER_GDEW0154D67
using LgfxDisplayPanel = lgfx::Panel_GDEW0154D67;
#elif FLXOS_DISPLAY_DRIVER_GDEW0154M09
using LgfxDisplayPanel = lgfx::Panel_GDEW0154M09;
#elif FLXOS_DISPLAY_DRIVER_HUB75
using LgfxDisplayPanel = lgfx::Panel_HUB75;
#elif FLXOS_DISPLAY_DRIVER_IT8951
using LgfxDisplayPanel = lgfx::Panel_IT8951;
#elif FLXOS_DISPLAY_DRIVER_M5HDMI
using LgfxDisplayPanel = lgfx::Panel_M5HDMI;
#elif FLXOS_DISPLAY_DRIVER_M5UNITGLASS
using LgfxDisplayPanel = lgfx::Panel_M5UnitGLASS;
#elif FLXOS_DISPLAY_DRIVER_M5UNITLCD
using LgfxDisplayPanel = lgfx::Panel_M5UnitLCD;
#elif FLXOS_DISPLAY_DRIVER_RGB
using LgfxDisplayPanel = lgfx::Panel_RGB;
#elif FLXOS_DISPLAY_DRIVER_ST7701
using LgfxDisplayPanel = lgfx::Panel_ST7701;
#elif FLXOS_DISPLAY_DRIVER_GC9503
using LgfxDisplayPanel = lgfx::Panel_GC9503;
#elif FLXOS_DISPLAY_DRIVER_ILI9881C
using LgfxDisplayPanel = lgfx::Panel_ILI9881C;
#elif FLXOS_DISPLAY_DRIVER_HX8357D
using LgfxDisplayPanel = lgfx::Panel_HX8357D;
#elif FLXOS_DISPLAY_DRIVER_HX8357B
using LgfxDisplayPanel = lgfx::Panel_HX8357B;
#else
#error "FlxOS: Unsupported or missing hardware.display.driver in profile.yaml"
#endif

// ============================================================================
// Display Bus Dispatch
// ============================================================================
#if FLXOS_DISPLAY_BUS_SPI
using LgfxDisplayBus = lgfx::Bus_SPI;
#elif FLXOS_DISPLAY_BUS_I2C
using LgfxDisplayBus = lgfx::Bus_I2C;
#elif FLXOS_DISPLAY_BUS_PARALLEL8
using LgfxDisplayBus = lgfx::Bus_Parallel8;
#elif FLXOS_DISPLAY_BUS_PARALLEL16
using LgfxDisplayBus = lgfx::Bus_Parallel16;
#elif FLXOS_DISPLAY_BUS_RGB
using LgfxDisplayBus = lgfx::Bus_RGB;
#elif FLXOS_DISPLAY_BUS_MIPI_DSI
using LgfxDisplayBus = lgfx::Bus_DSI;
#elif FLXOS_DISPLAY_BUS_EPDIY
using LgfxDisplayBus = lgfx::Bus_SPI; // Dummy bus type for EPDiy
#else
#error "FlxOS: Unsupported or missing hardware.display.bus in profile.yaml"
#endif

// ============================================================================
// Touch Driver Dispatch
// ============================================================================
#if FLXOS_TOUCH_DRIVER_XPT2046
using LgfxTouch = lgfx::Touch_XPT2046;
#elif FLXOS_TOUCH_DRIVER_STMPE610
using LgfxTouch = lgfx::Touch_STMPE610;
#elif FLXOS_TOUCH_DRIVER_RA8875
using LgfxTouch = lgfx::Touch_RA8875;
#elif FLXOS_TOUCH_DRIVER_GT911
using LgfxTouch = lgfx::Touch_GT911;
#elif FLXOS_TOUCH_DRIVER_FT5X06
using LgfxTouch = lgfx::Touch_FT5x06;
#elif FLXOS_TOUCH_DRIVER_CSTXXX || FLXOS_TOUCH_DRIVER_CST816S
using LgfxTouch = lgfx::Touch_CST816S;
#elif FLXOS_TOUCH_DRIVER_CHSC6X || FLXOS_TOUCH_DRIVER_CHSC5816
using LgfxTouch = lgfx::Touch_CHSC6x;
#elif FLXOS_TOUCH_DRIVER_NS2009
using LgfxTouch = lgfx::Touch_NS2009;
#elif FLXOS_TOUCH_DRIVER_TT21XXX
using LgfxTouch = lgfx::Touch_TT21xxx;
#elif FLXOS_TOUCH_DRIVER_GSLX680 || FLXOS_TOUCH_DRIVER_GSL1680
using LgfxTouch = lgfx::Touch_GSLx680;
#else
#if FLXOS_HEADLESS == 0
// Provide a dummy touch type if none matched, it will only be instantiated if touch is enabled.
using LgfxTouch = lgfx::Touch_XPT2046;
#endif
#endif

// ============================================================================
// LovyanGFX Configuration Class
// ============================================================================
class LGFX : public lgfx::LGFX_Device {
	LgfxDisplayPanel _panel_instance;
	LgfxDisplayBus _bus_instance;
	lgfx::Light_PWM _light_instance;

	// Use conditional member variable so we don't compile missing touch deps if touch is disabled or unrecognized.
#if FLXOS_TOUCH_BUS_SPI || FLXOS_TOUCH_BUS_I2C
	LgfxTouch _touch_instance;
#endif

public:

	// Explicitly expose base methods
	int32_t width() const { return lgfx::LGFX_Device::width(); }
	int32_t height() const { return lgfx::LGFX_Device::height(); }
	uint8_t getRotation() const { return lgfx::LGFX_Device::getRotation(); }
	void setRotation(uint8_t r) { lgfx::LGFX_Device::setRotation(r); }

	LGFX(void) {
		// ====================================================================
		// Bus Configuration
		// ====================================================================
		{
			auto cfg = _bus_instance.config();

#if FLXOS_DISPLAY_BUS_SPI
			cfg.spi_host = flx::hal::detail::clampCast<decltype(cfg.spi_host)>(flx::config::display.spi.host);
			cfg.spi_mode = flx::hal::detail::clampCast<decltype(cfg.spi_mode)>(flx::config::display.spi.mode);
			cfg.freq_write = flx::config::display.spi.freqWrite;
			cfg.freq_read = flx::config::display.spi.freqRead;
			cfg.spi_3wire = flx::config::display.spi.threeWire;
			cfg.use_lock = true;
			cfg.dma_channel = flx::hal::detail::clampCast<decltype(cfg.dma_channel)>(
				(flx::config::display.spi.dmaChannel == 0) ? SPI_DMA_CH_AUTO : (flx::config::display.spi.dmaChannel == 1) ? 1
					: (flx::config::display.spi.dmaChannel == 2)														  ? 2
																														  : SPI_DMA_CH_AUTO);
			cfg.pin_sclk = flx::config::display.pins.sclk;
			cfg.pin_mosi = flx::config::display.pins.mosi;
			cfg.pin_miso = flx::config::display.pins.miso;
			cfg.pin_dc = flx::config::display.pins.dc;
#elif FLXOS_DISPLAY_BUS_I2C
			cfg.i2c_port = flx::config::display.i2c.port;
			cfg.i2c_addr = flx::hal::detail::clampCast<decltype(cfg.i2c_addr)>(flx::config::display.i2c.addr);
			cfg.pin_sda = flx::config::display.i2c.sda;
			cfg.pin_scl = flx::config::display.i2c.scl;
			cfg.freq_write = flx::config::display.i2c.freq;
			cfg.freq_read = flx::config::display.i2c.freq;
#elif FLXOS_DISPLAY_BUS_PARALLEL8
			cfg.freq_write = flx::config::display.parallel.freqWrite;
			cfg.freq_read = flx::config::display.parallel.freqRead;
			cfg.pin_rd = flx::config::display.parallel.rd;
			cfg.pin_wr = flx::config::display.parallel.wr;
			cfg.pin_rs = flx::config::display.parallel.rs;
			cfg.pin_d0 = flx::config::display.parallel.d0;
			cfg.pin_d1 = flx::config::display.parallel.d1;
			cfg.pin_d2 = flx::config::display.parallel.d2;
			cfg.pin_d3 = flx::config::display.parallel.d3;
			cfg.pin_d4 = flx::config::display.parallel.d4;
			cfg.pin_d5 = flx::config::display.parallel.d5;
			cfg.pin_d6 = flx::config::display.parallel.d6;
			cfg.pin_d7 = flx::config::display.parallel.d7;
#elif FLXOS_DISPLAY_BUS_PARALLEL16
			cfg.freq_write = flx::config::display.parallel.freqWrite;
			cfg.freq_read = flx::config::display.parallel.freqRead;
			cfg.pin_rd = flx::config::display.parallel.rd;
			cfg.pin_wr = flx::config::display.parallel.wr;
			cfg.pin_rs = flx::config::display.parallel.rs;
			cfg.pin_d0 = flx::config::display.parallel.d0;
			cfg.pin_d1 = flx::config::display.parallel.d1;
			cfg.pin_d2 = flx::config::display.parallel.d2;
			cfg.pin_d3 = flx::config::display.parallel.d3;
			cfg.pin_d4 = flx::config::display.parallel.d4;
			cfg.pin_d5 = flx::config::display.parallel.d5;
			cfg.pin_d6 = flx::config::display.parallel.d6;
			cfg.pin_d7 = flx::config::display.parallel.d7;
			cfg.pin_d8 = flx::config::display.parallel.d8;
			cfg.pin_d9 = flx::config::display.parallel.d9;
			cfg.pin_d10 = flx::config::display.parallel.d10;
			cfg.pin_d11 = flx::config::display.parallel.d11;
			cfg.pin_d12 = flx::config::display.parallel.d12;
			cfg.pin_d13 = flx::config::display.parallel.d13;
			cfg.pin_d14 = flx::config::display.parallel.d14;
			cfg.pin_d15 = flx::config::display.parallel.d15;
#elif FLXOS_DISPLAY_BUS_RGB
			cfg.freq_write = flx::config::display.rgb.freqWrite;
			cfg.pin_pclk = flx::config::display.rgb.pclk;
			cfg.pin_vsync = flx::config::display.rgb.vsync;
			cfg.pin_hsync = flx::config::display.rgb.hsync;
			cfg.pin_henable = flx::config::display.rgb.henable;
			cfg.pin_d0 = flx::config::display.rgb.d0;
			cfg.pin_d1 = flx::config::display.rgb.d1;
			cfg.pin_d2 = flx::config::display.rgb.d2;
			cfg.pin_d3 = flx::config::display.rgb.d3;
			cfg.pin_d4 = flx::config::display.rgb.d4;
			cfg.pin_d5 = flx::config::display.rgb.d5;
			cfg.pin_d6 = flx::config::display.rgb.d6;
			cfg.pin_d7 = flx::config::display.rgb.d7;
			cfg.pin_d8 = flx::config::display.rgb.d8;
			cfg.pin_d9 = flx::config::display.rgb.d9;
			cfg.pin_d10 = flx::config::display.rgb.d10;
			cfg.pin_d11 = flx::config::display.rgb.d11;
			cfg.pin_d12 = flx::config::display.rgb.d12;
			cfg.pin_d13 = flx::config::display.rgb.d13;
			cfg.pin_d14 = flx::config::display.rgb.d14;
			cfg.pin_d15 = flx::config::display.rgb.d15;
			cfg.hsync_pulse_width = flx::config::display.rgb.hsyncPulseWidth;
			cfg.hsync_back_porch = flx::config::display.rgb.hsyncBackPorch;
			cfg.hsync_front_porch = flx::config::display.rgb.hsyncFrontPorch;
			cfg.vsync_pulse_width = flx::config::display.rgb.vsyncPulseWidth;
			cfg.vsync_back_porch = flx::config::display.rgb.vsyncBackPorch;
			cfg.vsync_front_porch = flx::config::display.rgb.vsyncFrontPorch;
			cfg.hsync_polarity = flx::config::display.rgb.hsyncPolarity;
			cfg.vsync_polarity = flx::config::display.rgb.vsyncPolarity;
			cfg.pclk_active_neg = flx::config::display.rgb.pclkActiveNeg;
			cfg.de_idle_high = flx::config::display.rgb.deIdleHigh;
			cfg.pclk_idle_high = flx::config::display.rgb.pclkIdleHigh;
#elif FLXOS_DISPLAY_BUS_MIPI_DSI
			cfg.bus_id = 0;
			cfg.lane_num = 2;
			cfg.lane_mbps = 960;
#endif

			_bus_instance.config(cfg);
			_panel_instance.setBus(&_bus_instance);
		}

		// ====================================================================
		// Panel Configuration
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
			cfg.offset_rotation =
				flx::hal::detail::clampCast<decltype(cfg.offset_rotation)>(flx::config::display.panel.offsetRotation);
			cfg.dummy_read_pixel =
				flx::hal::detail::clampCast<decltype(cfg.dummy_read_pixel)>(flx::config::display.panel.dummyReadPixel);
			cfg.dummy_read_bits =
				flx::hal::detail::clampCast<decltype(cfg.dummy_read_bits)>(flx::config::display.panel.dummyReadBits);
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
			cfg.pwm_channel =
				flx::hal::detail::clampCast<decltype(cfg.pwm_channel)>(flx::config::display.backlight.pwmChannel);
			_light_instance.config(cfg);
			_panel_instance.setLight(&_light_instance);
		}

#if FLXOS_TOUCH_BUS_SPI || FLXOS_TOUCH_BUS_I2C
		if constexpr (flx::config::touch.enabled) {
			// ====================================================================
			// Touch Configuration
			// ====================================================================
			{
				auto cfg = _touch_instance.config();
				cfg.x_min = flx::config::touch.calibration.xMin;
				cfg.x_max = flx::config::touch.calibration.xMax;
				cfg.y_min = flx::config::touch.calibration.yMin;
				cfg.y_max = flx::config::touch.calibration.yMax;
				cfg.pin_int = flx::config::touch.pins.interrupt;
				cfg.offset_rotation =
					flx::hal::detail::clampCast<decltype(cfg.offset_rotation)>(flx::config::touch.calibration.offsetRotation);

#if FLXOS_TOUCH_BUS_SPI
				cfg.bus_shared = flx::config::touch.spi.busShared;
				if constexpr (flx::config::touch.spi.separatePins) {
					// Dedicated touch SPI pins/host
					cfg.spi_host = flx::hal::detail::clampCast<decltype(cfg.spi_host)>(flx::config::touch.spi.host);
					cfg.pin_sclk = flx::config::touch.pins.sclk;
					cfg.pin_mosi = flx::config::touch.pins.mosi;
					cfg.pin_miso = flx::config::touch.pins.miso;
				} else {
					// Reuse display SPI
#if FLXOS_DISPLAY_BUS_SPI
					cfg.spi_host = flx::hal::detail::clampCast<decltype(cfg.spi_host)>(flx::config::display.spi.host);
					cfg.pin_sclk = flx::config::display.pins.sclk;
					cfg.pin_mosi = flx::config::display.pins.mosi;
					cfg.pin_miso = flx::config::display.pins.miso;
#endif
				}
				cfg.freq = flx::config::touch.spi.freq;
				cfg.pin_cs = flx::config::touch.pins.cs;
#elif FLXOS_TOUCH_BUS_I2C
				cfg.i2c_port = flx::config::touch.i2c.port;
				cfg.i2c_addr = flx::hal::detail::clampCast<decltype(cfg.i2c_addr)>(flx::config::touch.i2c.addr);
				cfg.pin_sda = flx::config::touch.i2c.sda;
				cfg.pin_scl = flx::config::touch.i2c.scl;
				cfg.freq = flx::config::touch.spi.freq; // Lovyan uses freq field regardless of union
#endif

				_touch_instance.config(cfg);
				_panel_instance.setTouch(&_touch_instance);
			}
		}
#endif

		setPanel(&_panel_instance);
	}
};

typedef struct {
	LGFX* tft;
} lv_lovyan_gfx_driver_data_t;
