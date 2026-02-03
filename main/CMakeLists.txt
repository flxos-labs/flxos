# Core sources (always included)
set(CORE_SRCS
    "main.cpp"
    "core/connectivity/ConnectivityManager.cpp"
    "core/connectivity/bluetooth/BluetoothManager.cpp"
    "core/connectivity/hotspot/HotspotManager.cpp"
    "core/connectivity/wifi/WiFiManager.cpp"
    "core/services/filesystem/FileSystemService.cpp"
    "core/services/systeminfo/SystemInfoService.cpp"
    "core/system/System/SystemManager.cpp"
    "core/system/Settings/SettingsManager.cpp"
    "core/system/Display/DisplayManager.cpp"
    "core/system/Theme/ThemeManager.cpp"
    "core/system/Time/TimeManager.cpp"
    "core/tasks/TaskManager.cpp"
    "core/tasks/resource_monitor/ResourceMonitorTask.cpp"
)

# GUI sources (excluded in headless mode)
message(STATUS "FlxOS Main: CONFIG_FLXOS_HEADLESS_MODE is '${CONFIG_FLXOS_HEADLESS_MODE}'")
if(NOT CONFIG_FLXOS_HEADLESS_MODE)
    list(APPEND CORE_SRCS
        "core/tasks/gui/GuiTask.cpp"
        "core/ui/DE/modules/StatusBar/StatusBar.cpp"
        "core/ui/DE/modules/Dock/Dock.cpp"
        "core/ui/DE/modules/NotificationPanel/NotificationPanel.cpp"
        "core/ui/DE/modules/QuickAccessPanel/QuickAccessPanel.cpp"
        "core/ui/DE/modules/Launcher/Launcher.cpp"
        "core/ui/DE/modules/SwipeManager/SwipeManager.cpp"
        "core/ui/DE/DE.cpp"
        "core/ui/DE/wm/WM.cpp"
        "core/ui/Keyboard/VirtualKeyboard.cpp"
        "core/ui/theming/ThemeEngine/ThemeEngine.cpp"
        "core/ui/theming/Themes/Themes.cpp"
        "core/apps/AppManager.cpp"
        "core/apps/files/FilesApp.cpp"
        "core/apps/system_info/SystemInfoApp.cpp"
        "core/apps/settings/wifi/WiFiSettings.cpp"
        "core/apps/settings/hotspot/HotspotSettings.cpp"
        "core/system/Focus/FocusManager.cpp"
        "core/system/Notification/NotificationManager.cpp"
    )
endif()

# CLI sources (when enabled)
if(CONFIG_FLXOS_CLI_ENABLED)
    list(APPEND CORE_SRCS
        "core/services/cli/CliService.cpp"
    )
endif()

# Set include directories based on mode
if(CONFIG_FLXOS_HEADLESS_MODE)
    set(INCLUDE_DIRS "." "hal/os" "core/common")
    set(COMPONENT_REQUIRES dhcpserver)
else()
    set(INCLUDE_DIRS "." "hal/display" "hal/os" "core/common" "core/ui/components")
    set(COMPONENT_REQUIRES LovyanGFX lvgl dhcpserver)
endif()

# Console is always included because Kconfig values aren't reliably available during first CMake pass
set(PRIV_REQUIRES esp_psram fatfs wear_levelling spi_flash esp_wifi esp_event esp_netif lwip nvs_flash esp_system heap esp_timer freertos json console)

# Register the main component
idf_component_register(SRCS ${CORE_SRCS}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    PRIV_REQUIRES ${PRIV_REQUIRES}
    REQUIRES ${COMPONENT_REQUIRES}
)

# LVGL configuration (only in GUI mode)
if(NOT CONFIG_FLXOS_HEADLESS_MODE)
    idf_component_get_property(lvgl_lib lvgl COMPONENT_LIB)
    if(CONFIG_LV_OS_IDLE_PERCENT_CUSTOM)
        target_sources(${lvgl_lib} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hal/os/lv_os_custom.c)
    endif()

    if(CONFIG_LV_USE_LOVYAN_GFX)
        target_link_libraries(${lvgl_lib} PUBLIC idf::LovyanGFX)
    endif()
endif()

# Upload
fatfs_create_spiflash_image(system ../assets/system FLASH_IN_PROJECT)
fatfs_create_spiflash_image(data ../assets/data FLASH_IN_PROJECT)
