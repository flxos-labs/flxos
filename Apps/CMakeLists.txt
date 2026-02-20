if(DEFINED PROJECT_DIR)
    include(${PROJECT_DIR}/Buildscripts/headless.cmake)
else()
    include(${CMAKE_CURRENT_LIST_DIR}/../Buildscripts/headless.cmake)
endif()
flx_resolve_headless(_flx_headless_mode)

set(APPS_SRCS
    "Source/AppContext.cpp"
)

if(NOT _flx_headless_mode)
    list(APPEND APPS_SRCS
        "Source/AppManager.cpp"
        "Source/AppRegistry.cpp"
    )
endif()

idf_component_register(
    SRCS ${APPS_SRCS}
    INCLUDE_DIRS Include
    REQUIRES Core Kernel Services
    PRIV_REQUIRES freertos
)
