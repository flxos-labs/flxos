#ifndef FREERTOS_H
#define FREERTOS_H

#include <stdint.h>
#include <unistd.h>

#define pdMS_TO_TICKS(x) (x)
#define portMAX_DELAY 0xFFFFFFFF

typedef void* TaskHandle_t;
typedef void* SemaphoreHandle_t;

inline void vTaskDelay(uint32_t ticks) {
	usleep(ticks * 1000);
}

inline SemaphoreHandle_t xSemaphoreCreateMutex() {
	return (SemaphoreHandle_t)1;
}

inline int xSemaphoreTake(SemaphoreHandle_t x, uint32_t wait) {
	return 1;
}

inline int xSemaphoreGive(SemaphoreHandle_t x) {
	return 1;
}

#endif
