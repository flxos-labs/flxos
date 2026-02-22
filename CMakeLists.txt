# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)
# Build acceleration: ccache as compiler launcher
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    message(STATUS "FlxOS: ccache enabled → ${CCACHE_PROGRAM}")
endif()

# Include FlxOS buildscripts
include(${CMAKE_CURRENT_SOURCE_DIR}/Buildscripts/module.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/Buildscripts/version.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/Buildscripts/profile.cmake)

# Embed version information
flx_embed_version()

# Load profile: reads profile.yaml → generates Config.hpp + sdkconfig.profile
# Sets HEADLESS_MODE_ENABLED, SDKCONFIG_DEFAULTS, EXCLUDE_COMPONENTS
flx_load_profile()

# Register FlxOS modules as extra components
set(EXTRA_COMPONENT_DIRS
    "${CMAKE_SOURCE_DIR}/Core"
    "${CMAKE_SOURCE_DIR}/Kernel"
    "${CMAKE_SOURCE_DIR}/Services"
    "${CMAKE_SOURCE_DIR}/Apps"
    "${CMAKE_SOURCE_DIR}/HAL"
    "${CMAKE_SOURCE_DIR}/Applications"
    "${CMAKE_SOURCE_DIR}/Firmware"

    "${CMAKE_SOURCE_DIR}/Connectivity"
    "${CMAKE_SOURCE_DIR}/System"
    "${CMAKE_SOURCE_DIR}/UI"
    "${CMAKE_SOURCE_DIR}/Profiles"
)

if(HEADLESS_MODE_ENABLED)
    add_compile_definitions(LV_USE_LOVYAN_GFX=0)
else()
    add_compile_definitions(LV_USE_LOVYAN_GFX=1)
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(FlxOS)
