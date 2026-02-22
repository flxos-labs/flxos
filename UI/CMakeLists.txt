# UI component — LVGL UI framework, desktop, theming
if(NOT FLXOS_HEADLESS_MODE_ENABLED)
    file(GLOB_RECURSE UI_SOURCES "Source/*.cpp")

    idf_component_register(
        SRCS ${UI_SOURCES}
        INCLUDE_DIRS Include
        REQUIRES Core Kernel Services Apps Connectivity System lvgl
        PRIV_REQUIRES LovyanGFX HAL Profiles
    )

    target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_USE_LOVYAN_GFX=1)
else()
    idf_component_register(
        SRCS
        INCLUDE_DIRS Include
        REQUIRES Core Kernel Services Apps Connectivity System
    )
endif()
