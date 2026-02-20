if(DEFINED PROJECT_DIR)
    include(${PROJECT_DIR}/Buildscripts/headless.cmake)
else()
    include(${CMAKE_CURRENT_LIST_DIR}/../Buildscripts/headless.cmake)
endif()
flx_resolve_headless(_flx_headless_mode)

# Set component requirements based on build mode
set(SYSTEM_REQUIRES Core Services Kernel Connectivity Profiles wear_levelling esp_timer)
set(SYSTEM_SRCS
    "Source/SystemManager.cpp"
    "Source/managers/SettingsManager.cpp"
    "Source/managers/DisplayManager.cpp"
    "Source/managers/ThemeManager.cpp"
    "Source/managers/TimeManager.cpp"
    "Source/managers/PowerManager.cpp"
    "Source/services/SdCardService.cpp"
    "Source/services/DeviceProfileService.cpp"
    "Source/services/CliService.cpp"
    "Source/services/FileSystemService.cpp"
    "Source/services/SystemInfoService.cpp"
    "Source/device/DeviceProfiles.cpp"
)

if(NOT _flx_headless_mode)
    list(APPEND SYSTEM_REQUIRES lvgl)
    list(APPEND SYSTEM_SRCS
        "Source/managers/NotificationManager.cpp"
        "Source/services/ScreenshotService.cpp"
    )
endif()

idf_component_register(
    SRCS ${SYSTEM_SRCS}
    INCLUDE_DIRS Include
    REQUIRES ${SYSTEM_REQUIRES}
    PRIV_REQUIRES nvs_flash fatfs sdmmc spi_flash console json esp_driver_i2c esp_adc
)
