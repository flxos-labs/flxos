file(GLOB_RECURSE SRCS *.cpp)

# Register the main component
idf_component_register(SRCS ${SRCS}
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_psram fatfs wear_levelling spi_flash esp_wifi esp_event esp_netif lwip nvs_flash esp_system heap esp_timer freertos
    REQUIRES LovyanGFX lvgl dhcpserver
)

# Uploaded
fatfs_create_spiflash_image(system ../assets/system FLASH_IN_PROJECT)
fatfs_create_spiflash_image(data ../assets/data FLASH_IN_PROJECT)