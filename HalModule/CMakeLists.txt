# HalModule component — FlxOS Hardware Abstraction Layer
set(HALMODULE_REQUIRES Core)
set(HALMODULE_PRIV_REQUIRES fatfs sdmmc spi_flash esp_driver_spi esp_driver_i2c esp_driver_uart esp_driver_gpio driver Profiles)

# Use $ENV{...} to ensure this is visible during ESP-IDF's requirement discovery phase
if("$ENV{FLXOS_HEADLESS_MODE_ENABLED}" STREQUAL "ON")
    message("HALMODULE: HEADLESS MODE DETECTED (via ENV)")
else()
    message("HALMODULE: GRAPHICAL MODE DETECTED (via ENV)")
    list(APPEND HALMODULE_REQUIRES LovyanGFX)
    list(APPEND HALMODULE_PRIV_REQUIRES lvgl)
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

idf_component_register(
    SRCS ${HALMODULE_SRCS}
    INCLUDE_DIRS Include
    REQUIRES ${HALMODULE_REQUIRES}
    PRIV_REQUIRES ${HALMODULE_PRIV_REQUIRES}
)
