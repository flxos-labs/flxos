#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include <string.h>

#define globals LV_GLOBAL_DEFAULT()

uint32_t lv_os_get_idle_percent(void) {
#ifdef ESP_PLATFORM
	// ESP32 implementation using FreeRTOS idle task runtime stats
	static uint32_t last_total_time = 0;
	static uint32_t last_idle_time = 0;
	uint32_t current_idle_time = 0;
	uint32_t pct = 0;

	// On ESP-IDF SMP/Multicore, we have an idle task per core.
	// We can get the idle task handles and query their runtime statistics directly via vTaskGetInfo.
	// This is 100% thread-safe and avoids calling the expensive and race-prone uxTaskGetSystemState().
#ifdef CONFIG_FREERTOS_NUMBER_OF_CORES
	int const cores = CONFIG_FREERTOS_NUMBER_OF_CORES;
#else
	int const cores = 1;
#endif

	for (int core = 0; core < cores; core++) {
		TaskHandle_t idle_handle = NULL;
#if CONFIG_FREERTOS_NUMBER_OF_CORES > 1
		idle_handle = xTaskGetIdleTaskHandleForCore(core);
#else
		idle_handle = xTaskGetIdleTaskHandle();
#endif
		if (idle_handle != NULL) {
			TaskStatus_t task_status;
			// We only need the run time counter (set xGetFreeStackSpace = pdFALSE to avoid slow/dangerous stack scanning)
			vTaskGetInfo(idle_handle, &task_status, pdFALSE, eInvalid);
			current_idle_time += task_status.ulRunTimeCounter;
		}
	}

	uint32_t ulTotalRunTime = 0;
#ifdef portGET_RUN_TIME_COUNTER_VALUE
	ulTotalRunTime = portGET_RUN_TIME_COUNTER_VALUE();
#else
	ulTotalRunTime = (uint32_t)esp_timer_get_time();
#endif

	// Avoid division by zero and handle counter overflow
	if (ulTotalRunTime > last_total_time) {
		uint32_t total_delta = ulTotalRunTime - last_total_time;
		uint32_t idle_delta = current_idle_time - last_idle_time;

		// Normalize total time by number of cores as run time stats are per-core
		total_delta *= cores;

		// Clamp idle delta to total delta to avoid > 100% due to race conditions
		// in SMP stats
		if (idle_delta > total_delta)
			idle_delta = total_delta;

		pct = (idle_delta * 100) / total_delta;
	}
	last_total_time = ulTotalRunTime;
	last_idle_time = current_idle_time;

	return pct;
#else
	if (globals->freertos_non_idle_time_sum + globals->freertos_idle_time_sum ==
		0) {
		LV_LOG_WARN("Not enough time elapsed to provide idle percentage");
		return 0;
	}

	uint32_t pct =
		(globals->freertos_idle_time_sum * 100) /
		(globals->freertos_idle_time_sum + globals->freertos_non_idle_time_sum);

	globals->freertos_non_idle_time_sum = 0;
	globals->freertos_idle_time_sum = 0;

	return pct;
#endif
}
