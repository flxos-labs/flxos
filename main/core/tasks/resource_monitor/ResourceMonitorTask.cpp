#include "ResourceMonitorTask.hpp"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "ResourceMonitor";

namespace System {
ResourceMonitorTask& ResourceMonitorTask::getInstance() {
	static ResourceMonitorTask instance;
	return instance;
}

ResourceMonitorTask::ResourceMonitorTask()
	: Task("res_monitor", 4096, 2, tskNO_AFFINITY) {}

ResourceMonitorTask::Stats ResourceMonitorTask::getLatestStats() const {
	return {m_freeHeap.load(), m_minFreeHeap.load(), m_freePsram.load(), m_uptimeSeconds.load()};
}

void ResourceMonitorTask::run(void* data) {
	ESP_LOGI(TAG, "Resource Monitor Task Started");

	setWatchdogTimeout(15000);

	while (true) {
		heartbeat();

		m_freeHeap = esp_get_free_heap_size();
		m_minFreeHeap = esp_get_minimum_free_heap_size();
		m_freePsram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
		m_uptimeSeconds = (uint32_t)(esp_timer_get_time() / 1000000);

		ESP_LOGD(
			TAG, "Heap: %u, Min: %u, PSRAM: %u, Uptime: %u s",
			(unsigned int)m_freeHeap.load(), (unsigned int)m_minFreeHeap.load(),
			(unsigned int)m_freePsram.load(), (unsigned int)m_uptimeSeconds.load()
		);

		if (m_freeHeap < 32768) {
			ESP_LOGW(TAG, "LOW MEMORY WARNING: Free heap is %u bytes", (unsigned int)m_freeHeap.load());
		}
		vTaskDelay(pdMS_TO_TICKS(10000));
	}
}

} // namespace System
