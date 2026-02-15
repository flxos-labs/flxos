# Set component requirements based on build mode
set(SYSTEM_REQUIRES Core Services Kernel Connectivity wear_levelling esp_timer)
if(NOT CONFIG_FLXOS_HEADLESS_MODE)
    list(APPEND SYSTEM_REQUIRES lvgl)
endif()

idf_component_register(
    SRCS "Source/SystemManager.cpp"
         "Source/managers/SettingsManager.cpp"
         "Source/managers/DisplayManager.cpp"
         "Source/managers/ThemeManager.cpp"
         "Source/managers/TimeManager.cpp"
         "Source/managers/PowerManager.cpp"

         "Source/managers/NotificationManager.cpp"
         "Source/services/SdCardService.cpp"
         "Source/services/DeviceProfileService.cpp"
         "Source/services/ScreenshotService.cpp"
         "Source/services/CliService.cpp"
         "Source/services/FileSystemService.cpp"
         "Source/services/SystemInfoService.cpp"
         "Source/device/DeviceProfiles.cpp"
    INCLUDE_DIRS Include
    REQUIRES ${SYSTEM_REQUIRES}
    PRIV_REQUIRES nvs_flash fatfs sdmmc spi_flash console json
)
