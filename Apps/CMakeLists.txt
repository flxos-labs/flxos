set(APPS_SRCS
    "Source/AppContext.cpp"
)

if(NOT CONFIG_FLXOS_HEADLESS_MODE)
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
