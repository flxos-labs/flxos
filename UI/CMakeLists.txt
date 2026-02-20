if(DEFINED PROJECT_DIR)
    include(${PROJECT_DIR}/Buildscripts/headless.cmake)
else()
    include(${CMAKE_CURRENT_LIST_DIR}/../Buildscripts/headless.cmake)
endif()
flx_resolve_headless(_flx_headless_mode)

if(NOT _flx_headless_mode)
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
