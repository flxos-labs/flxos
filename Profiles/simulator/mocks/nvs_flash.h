#ifndef NVS_FLASH_H
#define NVS_FLASH_H

#include "esp_err.h"

#define ESP_ERR_NVS_NO_FREE_PAGES 0x1101
#define ESP_ERR_NVS_NEW_VERSION_FOUND 0x1102

inline esp_err_t nvs_flash_init() { return ESP_OK; }
inline esp_err_t nvs_flash_erase() { return ESP_OK; }

#endif
