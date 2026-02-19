#ifndef ESP_ERR_H
#define ESP_ERR_H

typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_NO_MEM 0x101
#define ESP_ERROR_CHECK(x) (void)(x)

#endif
