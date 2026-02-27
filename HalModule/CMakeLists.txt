# HalModule component — FlxOS Hardware Abstraction Layer
# Phases 0–15 per HAL_ULTIMATE_PLAN.md

set(HALMODULE_REQUIRES Core)
set(HALMODULE_PRIV_REQUIRES fatfs sdmmc spi_flash esp_driver_spi Profiles)

if(NOT FLXOS_HEADLESS_MODE_ENABLED)
    list(APPEND HALMODULE_REQUIRES LovyanGFX)
    list(APPEND HALMODULE_PRIV_REQUIRES lvgl)
endif()

# Collect all .cpp and .c files from the Source tree automatically.
# Add new subdirectories under Source/ freely — they're auto-included.
file(GLOB_RECURSE HALMODULE_SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.c"
)

idf_component_register(
    SRCS ${HALMODULE_SRCS}
    INCLUDE_DIRS Include
    REQUIRES ${HALMODULE_REQUIRES}
    PRIV_REQUIRES ${HALMODULE_PRIV_REQUIRES}
)
