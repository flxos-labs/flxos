# HalModule component — hardware abstraction (LovyanGFX wrapper)
set(HALMODULE_REQUIRES Core)
set(HALMODULE_PRIV_REQUIRES "")

if(NOT FLXOS_HEADLESS_MODE_ENABLED)
    list(APPEND HALMODULE_REQUIRES LovyanGFX)
    list(APPEND HALMODULE_PRIV_REQUIRES lvgl)
endif()

idf_component_register(
    SRCS ""
    INCLUDE_DIRS Include
    REQUIRES ${HALMODULE_REQUIRES}
    PRIV_REQUIRES ${HALMODULE_PRIV_REQUIRES}
)
