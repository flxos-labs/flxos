# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CCACHE_ENABLE 1)
endif()

# Include FlxOS buildscripts
include(${CMAKE_CURRENT_SOURCE_DIR}/Buildscripts/module.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/Buildscripts/version.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/Buildscripts/device.cmake)

# Embed version information
flx_embed_version()

# Read sdkconfig directly to check for headless mode before include()
# This is needed because CONFIG_* variables are not available until after include()
# EXCLUDE_COMPONENTS must be set BEFORE include($ENV{IDF_PATH}/tools/cmake/project.cmake)
if(EXISTS "${CMAKE_SOURCE_DIR}/sdkconfig")
    file(STRINGS "${CMAKE_SOURCE_DIR}/sdkconfig" SDKCONFIG_LINES)
    foreach(LINE ${SDKCONFIG_LINES})
        if(LINE MATCHES "^CONFIG_FLXOS_HEADLESS_MODE=y")
            message(STATUS "FlxOS: Headless mode enabled - excluding lvgl and LovyanGFX components")
            set(EXCLUDE_COMPONENTS lvgl LovyanGFX)
        endif()
    endforeach()
endif()

# Register FlxOS modules as extra components
set(EXTRA_COMPONENT_DIRS
    "${CMAKE_SOURCE_DIR}/Core"
    "${CMAKE_SOURCE_DIR}/Kernel"
    "${CMAKE_SOURCE_DIR}/Services"

    "${CMAKE_SOURCE_DIR}/Connectivity"
    "${CMAKE_SOURCE_DIR}/System"
    "${CMAKE_SOURCE_DIR}/UI"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(FlxOS)
