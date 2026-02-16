file(GLOB_RECURSE UI_SOURCES "Source/*.cpp")

idf_component_register(
    SRCS ${UI_SOURCES}
    INCLUDE_DIRS Include
    REQUIRES Core Kernel Services Apps Connectivity System HAL lvgl
    PRIV_REQUIRES LovyanGFX
)
