idf_component_register(
    SRCS "Source/lv_os_custom.c"
    INCLUDE_DIRS Include
    REQUIRES Core LovyanGFX
    PRIV_REQUIRES lvgl
)
