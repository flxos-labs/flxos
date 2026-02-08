# Core sources (always included)
set(CORE_SRCS
    "main.cpp"
    "core/connectivity/ConnectivityManager.cpp"
    "core/connectivity/bluetooth/BluetoothManager.cpp"
    "core/connectivity/hotspot/HotspotManager.cpp"
    "core/connectivity/wifi/WiFiManager.cpp"
    "core/services/filesystem/FileSystemService.cpp"
    "core/services/system_info/SystemInfoService.cpp"
    "core/system/system_core/SystemManager.cpp"
    "core/system/settings/SettingsManager.cpp"
    "core/system/display/DisplayManager.cpp"
    "core/system/theme/ThemeManager.cpp"
    "core/system/time/TimeManager.cpp"
    "core/system/power/PowerManager.cpp"
    "core/tasks/TaskManager.cpp"
    "core/tasks/resource_monitor/ResourceMonitorTask.cpp"
)

# GUI sources (excluded in headless mode)
message(STATUS "FlxOS Main: CONFIG_FLXOS_HEADLESS_MODE is '${CONFIG_FLXOS_HEADLESS_MODE}'")
if(NOT CONFIG_FLXOS_HEADLESS_MODE)
    list(APPEND CORE_SRCS
        "core/tasks/gui/GuiTask.cpp"
        "core/ui/desktop/modules/status_bar/StatusBar.cpp"
        "core/ui/desktop/modules/dock/Dock.cpp"
        "core/ui/desktop/modules/notification_panel/NotificationPanel.cpp"
        "core/ui/desktop/modules/quick_access_panel/QuickAccessPanel.cpp"
        "core/ui/desktop/modules/launcher/Launcher.cpp"
        "core/ui/desktop/modules/swipe_manager/SwipeManager.cpp"
        "core/ui/desktop/Desktop.cpp"
        "core/ui/desktop/window_manager/WindowManager.cpp"
        "core/ui/keyboard/VirtualKeyboard.cpp"
        "core/ui/theming/theme_engine/ThemeEngine.cpp"
        "core/ui/theming/themes/Themes.cpp"
        "core/apps/AppManager.cpp"
        "core/apps/files/FilesApp.cpp"
        "core/apps/system_info/SystemInfoApp.cpp"
        "core/apps/calendar/CalendarApp.cpp"
        "core/apps/settings/wifi/WiFiSettings.cpp"
        "core/apps/settings/hotspot/HotspotSettings.cpp"
        "core/apps/text_editor/TextEditorApp.cpp"
        "core/system/focus/FocusManager.cpp"
        "core/system/notification/NotificationManager.cpp"
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
