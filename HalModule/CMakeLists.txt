# HalModule component — FlxOS Hardware Abstraction Layer
set(HALMODULE_REQUIRES Core)
set(HALMODULE_PRIV_REQUIRES fatfs sdmmc spi_flash esp_driver_spi esp_driver_i2c esp_driver_uart esp_driver_gpio driver Profiles)

# Use $ENV{...} to ensure this is visible during ESP-IDF's requirement discovery phase
if("$ENV{FLXOS_HEADLESS_MODE_ENABLED}" STREQUAL "ON")
    message("HALMODULE: HEADLESS MODE DETECTED (via ENV)")
else()
    message("HALMODULE: GRAPHICAL MODE DETECTED (via ENV)")
    list(APPEND HALMODULE_REQUIRES LovyanGFX lvgl)
endif()

file(GLOB_RECURSE HALMODULE_SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.c"
)

# Always exclude lv_os_custom.c from HalModule
list(FILTER HALMODULE_SRCS EXCLUDE REGEX "Source/lv_os_custom.c")

if("$ENV{FLXOS_HEADLESS_MODE_ENABLED}" STREQUAL "ON")
    # Exclude display, touch, and LVGL-dependent input drivers from headless build
    list(FILTER HALMODULE_SRCS EXCLUDE REGEX "Source/display/.*")
    list(FILTER HALMODULE_SRCS EXCLUDE REGEX "Source/touch/.*")
    list(FILTER HALMODULE_SRCS EXCLUDE REGEX "Source/input/.*")
endif()

# ── Profile-driven conditional compilation ──
# Keyboard drivers: compile only the specific driver matching the profile
if(NOT FLXOS_HAS_KEYBOARD STREQUAL "true")
    # No keyboard at all — exclude all input drivers
    list(FILTER HALMODULE_SRCS EXCLUDE REGEX "Source/input/.*")
else()
    # Only compile keyboard drivers if they match the active keyboard type
    if(NOT FLXOS_KEYBOARD_TYPE STREQUAL "gpio")
        list(FILTER HALMODULE_SRCS EXCLUDE REGEX "Source/input/GpioKeyboardDevice\\.cpp")
    endif()
    if(NOT FLXOS_KEYBOARD_TYPE STREQUAL "tca8418")
        list(FILTER HALMODULE_SRCS EXCLUDE REGEX "Source/input/CardputerAdvKeyboard\\.cpp")
    endif()
    if(NOT FLXOS_KEYBOARD_TYPE STREQUAL "matrix_demux")
        list(FILTER HALMODULE_SRCS EXCLUDE REGEX "Source/input/CardputerMatrixKeyboard\\.cpp")
    endif()
endif()

# GPS drivers
if(NOT FLXOS_HAS_GPS STREQUAL "true")
    list(FILTER HALMODULE_SRCS EXCLUDE REGEX "Source/gps/.*")
endif()

# SD card drivers: compile only the matching bus variant
if(NOT FLXOS_HAS_SD STREQUAL "true")
    list(FILTER HALMODULE_SRCS EXCLUDE REGEX "Source/sdcard/.*")
else()
    if(NOT FLXOS_SD_BUS STREQUAL "sdmmc")
        list(FILTER HALMODULE_SRCS EXCLUDE REGEX "Source/sdcard/SdmmcSdCardDevice\\.cpp")
    endif()
    if(NOT FLXOS_SD_BUS STREQUAL "spi")
        list(FILTER HALMODULE_SRCS EXCLUDE REGEX "Source/sdcard/SpiSdCardDevice\\.cpp")
    endif()
endif()

# USB drivers
if(NOT FLXOS_HAS_TINYUSB STREQUAL "true")
    list(FILTER HALMODULE_SRCS EXCLUDE REGEX "Source/usb/.*")
endif()

list(LENGTH HALMODULE_SRCS _hal_src_count)
message(STATUS "HALMODULE: ${_hal_src_count} sources after profile filtering"
    " (kbd=${FLXOS_KEYBOARD_TYPE}, gps=${FLXOS_HAS_GPS}, sd=${FLXOS_SD_BUS}, usb=${FLXOS_HAS_TINYUSB})")

idf_component_register(
    SRCS ${HALMODULE_SRCS}
    INCLUDE_DIRS Include
    REQUIRES ${HALMODULE_REQUIRES}
    PRIV_REQUIRES ${HALMODULE_PRIV_REQUIRES}
)
