# System component — FlxOS System Services and Managers
set(SYSTEM_REQUIRES Core Services Kernel Connectivity Profiles wear_levelling esp_timer HalModule Apps esp_http_server)
set(SYSTEM_SRCS
    "Source/SystemManager.cpp"
    "Source/SystemDiagnostics.cpp"
    "Source/managers/SettingsManager.cpp"
    "Source/managers/TimeManager.cpp"
    "Source/managers/PowerManager.cpp"
    "Source/services/SdCardService.cpp"
    "Source/services/DeviceProfileService.cpp"
    "Source/services/CliService.cpp"
    "Source/services/FileSystemService.cpp"
    "Source/services/SystemInfoService.cpp"
    "Source/services/HalInitService.cpp"
    "Source/services/WebServerService.cpp"
)

if(NOT "$ENV{FLXOS_HEADLESS_MODE_ENABLED}" STREQUAL "ON")
    message("SYSTEM: GRAPHICAL MODE DETECTED (via ENV)")
    list(APPEND SYSTEM_REQUIRES lvgl FlxApp)
    list(APPEND SYSTEM_SRCS
        "Source/managers/DisplayManager.cpp"
        "Source/managers/ThemeManager.cpp"
        "Source/managers/WallpaperManager.cpp"
        "Source/managers/NotificationManager.cpp"
        "Source/services/ScreenRecorderService.cpp"
        "Source/services/ScreenshotService.cpp"
    )
else()
    message("SYSTEM: HEADLESS MODE DETECTED (via ENV)")
endif()

idf_component_register(
    SRCS ${SYSTEM_SRCS}
    INCLUDE_DIRS Include
    REQUIRES ${SYSTEM_REQUIRES}
    PRIV_REQUIRES nvs_flash fatfs sdmmc spi_flash console esp_driver_i2c esp_adc
)
