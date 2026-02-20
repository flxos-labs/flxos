if(DEFINED PROJECT_DIR)
    include(${PROJECT_DIR}/Buildscripts/headless.cmake)
else()
    include(${CMAKE_CURRENT_LIST_DIR}/../Buildscripts/headless.cmake)
endif()
flx_resolve_headless(_flx_headless_mode)

set(HAL_REQUIRES Core)
set(HAL_PRIV_REQUIRES "")

if(NOT _flx_headless_mode)
    list(APPEND HAL_REQUIRES LovyanGFX)
    list(APPEND HAL_PRIV_REQUIRES lvgl)
endif()

idf_component_register(
    SRCS ""
    INCLUDE_DIRS Include
    REQUIRES ${HAL_REQUIRES}
    PRIV_REQUIRES ${HAL_PRIV_REQUIRES}
)
