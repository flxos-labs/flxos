# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CCACHE_ENABLE 1)
endif()

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

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(FlxOS)
