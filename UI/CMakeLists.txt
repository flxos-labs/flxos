file(GLOB_RECURSE UI_SOURCES "Source/*.cpp")

idf_component_register(
    SRCS ${UI_SOURCES}
    INCLUDE_DIRS Include
    REQUIRES Core Kernel Services Apps Connectivity System lvgl
    PRIV_REQUIRES LovyanGFX HAL Profiles
)

if(NOT CONFIG_FLXOS_HEADLESS_MODE)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_USE_LOVYAN_GFX=1)
endif()
