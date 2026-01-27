file(GLOB_RECURSE SRCS *.cpp)

# Register the main component
idf_component_register(SRCS ${SRCS}
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_psram fatfs wear_levelling spi_flash esp_wifi esp_event esp_netif lwip nvs_flash esp_system heap esp_timer freertos
    REQUIRES LovyanGFX lvgl dhcpserver
)

if(CONFIG_LV_USE_LOVYAN_GFX)
    idf_component_get_property(lvgl_lib lvgl COMPONENT_LIB)
    target_link_libraries(${lvgl_lib} PUBLIC idf::LovyanGFX)
endif()

# Uploaded
fatfs_create_spiflash_image(system ../assets/system FLASH_IN_PROJECT)
fatfs_create_spiflash_image(data ../assets/data FLASH_IN_PROJECT)