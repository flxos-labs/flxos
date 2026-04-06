cmake_minimum_required(VERSION 3.16)

# Build acceleration: ccache as compiler launcher
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    message(STATUS "FlxOS: ccache enabled → ${CCACHE_PROGRAM}")
endif()

# Load profile and determine headless mode early
# This must happen before including project.cmake
include("${CMAKE_SOURCE_DIR}/Buildscripts/profile.cmake")
flx_load_profile()

# ── Component Directory Discovery ──
# We explicitly list fundamental core modules.
set(EXTRA_COMPONENT_DIRS
    "${CMAKE_SOURCE_DIR}/Core"
    "${CMAKE_SOURCE_DIR}/Kernel"
    "${CMAKE_SOURCE_DIR}/Services"
    "${CMAKE_SOURCE_DIR}/Apps"
    "${CMAKE_SOURCE_DIR}/HalModule"
    "${CMAKE_SOURCE_DIR}/Firmware"
    "${CMAKE_SOURCE_DIR}/Connectivity"
    "${CMAKE_SOURCE_DIR}/System"
    "${CMAKE_SOURCE_DIR}/Profiles"
)

# ── Restricted Pull-based Component Model ──
# If COMPONENTS is set, IDF only builds what's listed + its dependencies.
set(COMPONENTS Firmware Profiles Connectivity)

if(FLXOS_HEADLESS_MODE_ENABLED STREQUAL "ON")
    message("ROOT: Headless mode active — isolating graphical modules")
    add_compile_definitions(LV_USE_LOVYAN_GFX=0)
    # We do NOT add UI or Applications to EXTRA_COMPONENT_DIRS or COMPONENTS
else()
    message("ROOT: Graphical mode active")
    add_compile_definitions(LV_USE_LOVYAN_GFX=1)
    
    # Add graphical folders and libraries to discovery path
    list(APPEND EXTRA_COMPONENT_DIRS 
        "${CMAKE_SOURCE_DIR}/UI"
        "${CMAKE_SOURCE_DIR}/Applications"
        "${CMAKE_SOURCE_DIR}/FlxApp"
        "${CMAKE_SOURCE_DIR}/Libraries"
    )
    
    # Explicitly pull them in as roots
    list(APPEND COMPONENTS UI Applications)
endif()

message("ROOT: HEADLESS_MODE_ENABLED = ${FLXOS_HEADLESS_MODE_ENABLED}")
message("ROOT: COMPONENTS = ${COMPONENTS}")
message("ROOT: EXTRA_COMPONENT_DIRS = ${EXTRA_COMPONENT_DIRS}")
message("ROOT: EXCLUDE_COMPONENTS = ${EXCLUDE_COMPONENTS}")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(FlxOS)

# ── Storage Partition Images ──
# Generate FAT images for the `system` and `data` partitions from the
# storage/ directories and include them in the standard `idf.py flash`.
# Uses wear-levelling (wl_fatfsgen.py) to match CONFIG_WL_SECTOR_SIZE / WL driver expectations.
fatfs_create_spiflash_image(system "${CMAKE_SOURCE_DIR}/storage/system" FLASH_IN_PROJECT)
fatfs_create_spiflash_image(data   "${CMAKE_SOURCE_DIR}/storage/data"   FLASH_IN_PROJECT)
