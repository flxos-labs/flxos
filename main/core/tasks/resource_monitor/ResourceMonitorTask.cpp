#include "ResourceMonitorTask.hpp"
#include "core/common/Logger.hpp"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/task.h"
#include <string_view>

static constexpr std::string_view TAG = "ResourceMonitor";

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

	setWatchdogTimeout(15000);

	while (true) {
		heartbeat();

		m_freeHeap = esp_get_free_heap_size();
		m_minFreeHeap = esp_get_minimum_free_heap_size();
		m_freePsram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
		m_uptimeSeconds = (uint32_t)(esp_timer_get_time() / 1000000);

		if (m_freeHeap < 32768) {
			Log::warn(TAG, "LOW HEAP MEMORY: %lu bytes", (uint32_t)m_freeHeap.load());
		}

		if (m_uptimeSeconds % 60 == 0) {
			Log::info(TAG, "Stats - Heap: %lu, PSRAM: %lu, Uptime: %lu s", (uint32_t)m_freeHeap.load(), (uint32_t)m_freePsram.load(), (uint32_t)m_uptimeSeconds.load());
		}
		vTaskDelay(pdMS_TO_TICKS(10000));
	}
}

} // namespace System
