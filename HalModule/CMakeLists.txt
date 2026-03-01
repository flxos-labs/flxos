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
if(CMAKE_SCRIPT_MODE_FILE)
    file(GLOB_RECURSE HALMODULE_SRCS
        "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.c"
    )
else()
    file(GLOB_RECURSE HALMODULE_SRCS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.c"
    )
endif()

idf_component_register(
    SRCS ${HALMODULE_SRCS}
    INCLUDE_DIRS Include
    REQUIRES ${HALMODULE_REQUIRES}
    PRIV_REQUIRES ${HALMODULE_PRIV_REQUIRES}
)
