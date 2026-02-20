if(DEFINED PROJECT_DIR)
    include(${PROJECT_DIR}/Buildscripts/headless.cmake)
else()
    include(${CMAKE_CURRENT_LIST_DIR}/../Buildscripts/headless.cmake)
endif()
flx_resolve_headless(_flx_headless_mode)

set(EXTRA_REQS "")
if(NOT _flx_headless_mode)
    list(APPEND EXTRA_REQS UI Applications)
endif()

idf_component_register(
    SRCS "../Firmware/Source/Main.cpp"
    INCLUDE_DIRS "."
    REQUIRES Core Kernel Services Apps Connectivity System HAL Firmware ${EXTRA_REQS}
)

if(NOT _flx_headless_mode AND CONFIG_LV_OS_IDLE_PERCENT_CUSTOM)
    idf_component_get_property(lvgl_lib lvgl COMPONENT_LIB)
    target_sources(${lvgl_lib} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../HAL/Source/lv_os_custom.c")
endif()
