#pragma once

// ============================================================================
// Display Configuration (ILI9341)
// ============================================================================
#define FLXOS_DISPLAY_DRIVER        "ILI9341"
#define FLXOS_DISPLAY_ILI9341       // Feature flag for LovyanGFX
#define FLXOS_BUS_SPI               // Feature flag for Bus Type

#define FLXOS_DISPLAY_WIDTH         320
#define FLXOS_DISPLAY_HEIGHT        240
#define FLXOS_DISPLAY_ROTATION      1
#define FLXOS_DISPLAY_INVERT        false
#define FLXOS_DISPLAY_RGB_ORDER     false

// Display Panel Settings
#define FLXOS_PANEL_OFFSET_X        0
#define FLXOS_PANEL_OFFSET_Y        0
#define FLXOS_PANEL_OFFSET_ROTATION 0
#define FLXOS_DUMMY_READ_PIXEL      8
#define FLXOS_DUMMY_READ_BITS       1
#define FLXOS_PANEL_READABLE        true
#define FLXOS_PANEL_DLEN_16BIT      false
#define FLXOS_BUS_SHARED            true

// Display Bus Settings (SPI)
#define FLXOS_SPI_HOST              SPI2_HOST
#define FLXOS_SPI_MODE              0
#define FLXOS_SPI_FREQ_WRITE        40000000
#define FLXOS_SPI_FREQ_READ         16000000
#define FLXOS_SPI_3WIRE             false
#define FLXOS_SPI_DMA_CHANNEL       0 // Auto

// Display Pins
#define FLXOS_PIN_CS                10
#define FLXOS_PIN_DC                9
#define FLXOS_PIN_RST               14
#define FLXOS_PIN_BUSY              -1
#define FLXOS_PIN_BCKL              21
#define FLXOS_PIN_MOSI              11
#define FLXOS_PIN_SCLK              12
#define FLXOS_PIN_MISO              13

// Backlight Settings
#define FLXOS_BCKL_INVERT           false
#define FLXOS_BCKL_FREQ             20000
#define FLXOS_BCKL_PWM_CHANNEL      0

// ============================================================================
// Touch Configuration (XPT2046)
// ============================================================================
#define FLXOS_TOUCH_DRIVER          "XPT2046"
#define FLXOS_TOUCH_XPT2046         // Feature flag for LovyanGFX
#define FLXOS_TOUCH_ENABLED         true // Explicitly enable touch

#define FLXOS_PIN_TOUCH_CS          5
#define FLXOS_PIN_TOUCH_INT         6
#define FLXOS_TOUCH_SPI_FREQ        1000000
#define FLXOS_TOUCH_BUS_SHARED      true

// Touch Calibration
#define FLXOS_TOUCH_X_MIN           200
#define FLXOS_TOUCH_X_MAX           3800
#define FLXOS_TOUCH_Y_MIN           200
#define FLXOS_TOUCH_Y_MAX           3800
#define FLXOS_TOUCH_OFFSET_ROTATION 0

// ============================================================================
// SD Card Configuration
// ============================================================================
#define FLXOS_SD_CS                 4
#define FLXOS_SD_MOUNT_POINT        "/sdcard"
#define FLXOS_SD_SPI_HOST           FLXOS_SPI_HOST
#define FLXOS_SD_MAX_FREQ_KHZ       20000
#define FLXOS_SD_PIN_MOSI           -1
#define FLXOS_SD_PIN_MISO           -1
#define FLXOS_SD_PIN_SCLK           -1

// ============================================================================
// Battery Configuration
// ============================================================================
#define FLXOS_BATTERY_ENABLED       false
// #define FLXOS_BATTERY_ADC_UNIT      1
// #define FLXOS_BATTERY_ADC_CHANNEL   7
// #define FLXOS_BATTERY_VOLTAGE_MAX   4200
// #define FLXOS_BATTERY_VOLTAGE_MIN   3300
// #define FLXOS_BATTERY_DIVIDER_FACTOR 200
