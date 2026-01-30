file(GLOB_RECURSE SRCS *.cpp)

# Register the main component
idf_component_register(SRCS ${SRCS}
    INCLUDE_DIRS "." "hal/display" "hal/os" "core/common" "core/ui/components"
    PRIV_REQUIRES esp_psram fatfs wear_levelling spi_flash esp_wifi esp_event esp_netif lwip nvs_flash esp_system heap esp_timer freertos
    REQUIRES LovyanGFX lvgl dhcpserver
)

# Enable C++ exceptions for crash recovery
target_compile_options(${COMPONENT_LIB} PRIVATE -fexceptions)
target_link_libraries(${COMPONENT_LIB} PRIVATE -fexceptions)

idf_component_get_property(lvgl_lib lvgl COMPONENT_LIB)
if(CONFIG_LV_OS_IDLE_PERCENT_CUSTOM)
    target_sources(${lvgl_lib} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/hal/os/lv_os_custom.c)
endif()

if(CONFIG_LV_USE_LOVYAN_GFX)
    target_link_libraries(${lvgl_lib} PUBLIC idf::LovyanGFX)
endif()

# Upload
fatfs_create_spiflash_image(system ../assets/system FLASH_IN_PROJECT)
fatfs_create_spiflash_image(data ../assets/data FLASH_IN_PROJECT)
