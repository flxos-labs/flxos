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

# Resolve whether headless mode is enabled before project() is included.
# CONFIG_* variables are unavailable at this stage, so inspect sdkconfig files directly.
set(HEADLESS_MODE_ENABLED OFF)
set(_flx_headless_scan_files
    "${CMAKE_SOURCE_DIR}/sdkconfig"
    "${CMAKE_SOURCE_DIR}/sdkconfig.defaults"
)

# Also scan the target-specific defaults file (e.g. sdkconfig.defaults.esp32s3).
# IDF_TARGET is set as a cmake cache variable by ESP-IDF before project() runs,
# but in early passes it might only exist in the environment.
set(_flx_idf_target "esp32")
if(DEFINED IDF_TARGET AND NOT IDF_TARGET STREQUAL "")
    set(_flx_idf_target "${IDF_TARGET}")
elseif(DEFINED ENV{IDF_TARGET} AND NOT "$ENV{IDF_TARGET}" STREQUAL "")
    set(_flx_idf_target "$ENV{IDF_TARGET}")
endif()
list(APPEND _flx_headless_scan_files "${CMAKE_SOURCE_DIR}/sdkconfig.defaults.${_flx_idf_target}")

# Respect externally supplied SDKCONFIG_DEFAULTS as additional signal sources.
if(DEFINED SDKCONFIG_DEFAULTS)
    foreach(_defaults_file IN LISTS SDKCONFIG_DEFAULTS)
        if(IS_ABSOLUTE "${_defaults_file}")
            list(APPEND _flx_headless_scan_files "${_defaults_file}")
        else()
            list(APPEND _flx_headless_scan_files "${CMAKE_SOURCE_DIR}/${_defaults_file}")
        endif()
    endforeach()
endif()

list(REMOVE_DUPLICATES _flx_headless_scan_files)
foreach(_scan_file IN LISTS _flx_headless_scan_files)
    if(EXISTS "${_scan_file}")
        file(STRINGS "${_scan_file}" _headless_lines REGEX "^CONFIG_FLXOS_HEADLESS_MODE=y$")
        if(_headless_lines)
            set(HEADLESS_MODE_ENABLED ON)
            break()
        endif()
    endif()
endforeach()

# Expose the resolved flag to all component CMakeLists.  During early CMake
# passes, CONFIG_FLXOS_HEADLESS_MODE may be unset, which can otherwise force
# non-headless dependency branches.
set(FLXOS_HEADLESS_MODE_ENABLED ${HEADLESS_MODE_ENABLED} CACHE INTERNAL "Resolved FlxOS headless mode")

if(HEADLESS_MODE_ENABLED)
    message(STATUS "FlxOS: Headless mode enabled - excluding lvgl and LovyanGFX components")
    list(APPEND EXCLUDE_COMPONENTS lvgl LovyanGFX)
    list(REMOVE_DUPLICATES EXCLUDE_COMPONENTS)
endif()

# Append LVGL defaults if not in headless mode
if(NOT HEADLESS_MODE_ENABLED)
    if(NOT SDKCONFIG_DEFAULTS)
        set(SDKCONFIG_DEFAULTS "sdkconfig.defaults")
    endif()
    list(APPEND SDKCONFIG_DEFAULTS "${CMAKE_SOURCE_DIR}/sdkconfig.lvgl.defaults")
endif()

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

if(NOT HEADLESS_MODE_ENABLED)
    add_compile_definitions(LV_USE_LOVYAN_GFX=1)
else()
    # Safety net: if lvgl still appears in the graph (e.g. stale sdkconfig/build cache),
    # force-disable the LovyanGFX driver path to avoid pulling lv_lgfx_user.hpp.
    add_compile_definitions(LV_USE_LOVYAN_GFX=0)
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(FlxOS)
