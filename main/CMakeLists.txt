# Core sources (always included)
set(CORE_SRCS
    "main.cpp"
)

# GUI sources (excluded in headless mode)
message(STATUS "FlxOS Main: CONFIG_FLXOS_HEADLESS_MODE is '${CONFIG_FLXOS_HEADLESS_MODE}'")
if(NOT CONFIG_FLXOS_HEADLESS_MODE)
    list(APPEND CORE_SRCS
        "core/apps/files/FilesApp.cpp"
        "core/apps/system_info/SystemInfoApp.cpp"
        "core/apps/calendar/CalendarApp.cpp"
        "core/apps/settings/SettingsApp.cpp"
        "core/apps/settings/wifi/WiFiSettings.cpp"
        "core/apps/settings/hotspot/HotspotSettings.cpp"
        "core/apps/text_editor/TextEditorApp.cpp"
        "core/apps/tools/ToolsApp.cpp"
        "core/apps/image_viewer/ImageViewerApp.cpp"
        "core/apps/tools/implementation/Calculator.cpp"
        "core/apps/tools/implementation/Stopwatch.cpp"
        "core/apps/tools/implementation/Flashlight.cpp"
        "core/apps/tools/implementation/DisplayTester.cpp"
        "core/apps/tools/implementation/Screenshot.cpp"
    )
endif()

# CLI sources (when enabled)
if(CONFIG_FLXOS_CLI_ENABLED)
    # CLI Service moved to System component
endif()

# SD Card sources (when enabled)
if(CONFIG_FLXOS_SD_CARD_ENABLED)
    # SD Card Service moved to System component
endif()

# Set include directories based on mode
if(CONFIG_FLXOS_HEADLESS_MODE)
    set(INCLUDE_DIRS "." "hal/os" "core/common")
    set(COMPONENT_REQUIRES dhcpserver)
else()
    set(INCLUDE_DIRS "." "hal/display" "hal/os" "core/common")
    set(COMPONENT_REQUIRES LovyanGFX lvgl dhcpserver)
endif()

# Console is always included because Kconfig values aren't reliably available during first CMake pass
set(PRIV_REQUIRES esp_psram fatfs wear_levelling spi_flash esp_wifi esp_event esp_netif lwip nvs_flash esp_system heap esp_timer freertos json console sdmmc)

# Register the main component
idf_component_register(SRCS ${CORE_SRCS}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    PRIV_REQUIRES ${PRIV_REQUIRES}
    REQUIRES ${COMPONENT_REQUIRES} Core Kernel Services System UI Connectivity
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
