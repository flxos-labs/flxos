set(FLXAPP_SRCS "")

if(FLXOS_HEADLESS_MODE_ENABLED STREQUAL "ON")
    message("FLXAPP: HEADLESS MODE DETECTED (ON)")
else()
    message("FLXAPP: GRAPHICAL MODE DETECTED (OFF)")
    set(FLXAPP_SRCS
        "Source/FlxApp.cpp"
        "Source/FlxAppManifest.cpp"
        "Source/FlxAppState.cpp"
        "Source/FlxAppRenderer.cpp"
        "Source/FlxAppActionRunner.cpp"
        "Source/FlxAppLoader.cpp"
        "Source/FlxAppBenchmark.cpp"
    )
endif()

idf_component_register(
    SRCS ${FLXAPP_SRCS}
    INCLUDE_DIRS Include
    REQUIRES Core Apps Services
    PRIV_REQUIRES lvgl esp_timer esp_http_client
)
