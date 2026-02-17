set(EXTRA_REQS "")
if(NOT CONFIG_FLXOS_HEADLESS_MODE)
    list(APPEND EXTRA_REQS UI Applications)
endif()

idf_component_register(
    SRCS "../Firmware/Source/Main.cpp"
    INCLUDE_DIRS "."
    REQUIRES Core Kernel Services Apps Connectivity System HAL Firmware ${EXTRA_REQS}
)

if(CONFIG_LV_OS_IDLE_PERCENT_CUSTOM)
    idf_component_get_property(lvgl_lib lvgl COMPONENT_LIB)
    target_sources(${lvgl_lib} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../HAL/Source/lv_os_custom.c")
endif()
