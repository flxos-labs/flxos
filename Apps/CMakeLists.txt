# Apps component — conditionally includes GUI app management
set(APPS_SRCS
    "Source/AppContext.cpp"
    "Source/AppPaths.cpp"
)

if(NOT "$ENV{FLXOS_HEADLESS_MODE_ENABLED}" STREQUAL "ON")
    message("APPS: GRAPHICAL MODE DETECTED (via ENV)")
    list(APPEND APPS_SRCS
        "Source/AppManager.cpp"
        "Source/AppRegistry.cpp"
        "Source/ContentResolver.cpp"
    )
else()
    message("APPS: HEADLESS MODE DETECTED (via ENV)")
endif()

idf_component_register(
    SRCS ${APPS_SRCS}
    INCLUDE_DIRS Include
    REQUIRES Core Kernel Services HalModule
    PRIV_REQUIRES freertos System
)
