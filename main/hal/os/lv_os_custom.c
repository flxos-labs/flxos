#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include <string.h>

static const char* TAG = "lv_os_custom";

#define globals LV_GLOBAL_DEFAULT()

uint32_t lv_os_get_idle_percent(void) {
#ifdef ESP_PLATFORM
	// ESP32 "Proper" implementation using FreeRTOS System State
	static uint32_t last_total_time = 0;
	static uint32_t last_idle_time = 0;

	TaskStatus_t* pxTaskStatusArray;
	volatile UBaseType_t uxArraySize, x;
	uint32_t ulTotalRunTime = 0;
	uint32_t current_idle_time = 0;
	uint32_t pct = 0;

	// Take a snapshot of the number of tasks in case it changes while we are
	// accessing the array
	uxArraySize = uxTaskGetNumberOfTasks();
	ESP_LOGD(TAG, "Calculating idle percent for %d tasks", (int)uxArraySize);
	pxTaskStatusArray = lv_malloc(uxArraySize * sizeof(TaskStatus_t));

	if (pxTaskStatusArray != NULL) {
		// Generate raw status information about each task.
		uxArraySize =
			uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);

		// Iterate through the tasks to find the IDLE task(s)
		for (x = 0; x < uxArraySize; x++) {
			// Check for IDLE task name (usually "IDLE" or "IDLE0", "IDLE1" on SMP)
			if (strncmp(pxTaskStatusArray[x].pcTaskName, "IDLE", 4) == 0) {
				current_idle_time += pxTaskStatusArray[x].ulRunTimeCounter;
			}
		}

		// Avoid division by zero and handle counter overflow
		if (ulTotalRunTime > last_total_time) {
			uint32_t total_delta = ulTotalRunTime - last_total_time;
			uint32_t idle_delta = current_idle_time - last_idle_time;

// Normalize total time by number of cores as run time stats are per-core
#ifdef CONFIG_FREERTOS_NUMBER_OF_CORES
			total_delta *= CONFIG_FREERTOS_NUMBER_OF_CORES;
#endif

			// Clamp idle delta to total delta to avoid > 100% due to race conditions
			// in SMP stats
			if (idle_delta > total_delta)
				idle_delta = total_delta;

			pct = (idle_delta * 100) / total_delta;
			ESP_LOGD(TAG, "Idle percent calculated: %d%% (idle_delta: %u, total_delta: %u)", (int)pct, (unsigned int)idle_delta, (unsigned int)total_delta);
		}
		last_total_time = ulTotalRunTime;
		last_idle_time = current_idle_time;

		lv_free(pxTaskStatusArray);
	}

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
