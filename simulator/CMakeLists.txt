cmake_minimum_required(VERSION 3.16)
project(flxos_simulator)

set(CMAKE_CXX_STANDARD 17)

# Enable ccache if available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

# Disable heavy LVGL components not needed for simulator
set(LV_CONF_BUILD_DISABLE_EXAMPLES ON CACHE BOOL "Disable LVGL examples")
set(LV_CONF_BUILD_DISABLE_DEMOS ON CACHE BOOL "Disable LVGL demos")
set(LV_USE_DEMO_WIDGETS OFF CACHE BOOL "")
set(LV_USE_DEMO_MUSIC OFF CACHE BOOL "")
set(LV_USE_DEMO_BENCHMARK OFF CACHE BOOL "")

# Add mocks and LVGL to include path
include_directories(
    mocks
    mocks/freertos
    ../main
    ../main/core/common
    ../components/lvgl
    .
)

# Find SDL2
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# Add LVGL as a subdirectory
add_subdirectory(../components/lvgl lvgl)

add_executable(flxos_sim
    main_sim.cpp
    LvglSdlHAL.cpp
    mocks/Logger_mock.cpp
)

target_link_libraries(flxos_sim SDL2::SDL2 lvgl)
target_compile_definitions(flxos_sim PRIVATE FLXOS_SIMULATOR LV_CONF_INCLUDE_SIMPLE)
