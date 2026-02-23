# HAL component — hardware abstraction (LovyanGFX wrapper)
set(HAL_REQUIRES Core)
set(HAL_PRIV_REQUIRES "")

if(NOT FLXOS_HEADLESS_MODE_ENABLED)
    list(APPEND HAL_REQUIRES LovyanGFX)
    list(APPEND HAL_PRIV_REQUIRES lvgl)
endif()

idf_component_register(
    SRCS ""
    INCLUDE_DIRS Include
    REQUIRES ${HAL_REQUIRES}
    PRIV_REQUIRES ${HAL_PRIV_REQUIRES}
)
