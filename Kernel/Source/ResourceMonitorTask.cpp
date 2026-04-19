#include "esp_heap_caps.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <flx/core/Bundle.hpp>
#include <flx/core/EventBus.hpp>
#include <flx/core/Logger.hpp>
#include <flx/kernel/ResourceMonitorTask.hpp>
#include <string_view>

static constexpr std::string_view TAG = "ResourceMonitor";

namespace {

const flx::core::Bundle& lowMemoryNotificationTemplate() {
	static const flx::core::Bundle kTemplate = [] {
		flx::core::Bundle data;
		data.putString("title", "Low Memory");
		data.putString("message", "System resources are critically low");
		data.putString("icon", "warning");
		data.putInt32("priority", 2);
		return data;
	}();
	return kTemplate;
}

} // namespace

namespace flx::kernel {
ResourceMonitorTask& ResourceMonitorTask::getInstance() {
	static ResourceMonitorTask instance;
	return instance;
}

ResourceMonitorTask::ResourceMonitorTask()
	: Task("res_monitor", 6144, 2, tskNO_AFFINITY) {}

ResourceMonitorTask::Stats ResourceMonitorTask::getLatestStats() const {
	return {m_freeHeap.load(), m_minFreeHeap.load(), m_freePsram.load(), m_uptimeSeconds.load()};
}

void ResourceMonitorTask::run(void* /*data*/) {

	setWatchdogTimeout(15000);

	bool wasLowMemory = false;
	uint32_t lastLowMemoryNotifyAt = 0;
	static constexpr uint32_t LOW_MEMORY_NOTIFY_COOLDOWN_SECONDS = 300;

	while (true) {
		heartbeat();

		m_freeHeap = esp_get_free_heap_size();
		m_minFreeHeap = esp_get_minimum_free_heap_size();
		m_freePsram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
		m_uptimeSeconds = (uint32_t)(esp_timer_get_time() / 1000000);

		// PowerManager refresh removed to decouple Kernel from System.
		// TODO: Implement self-updating mechanism in PowerManager or use EventBus.

		if (m_freeHeap < 32768) {
			Log::warn(TAG, "LOW HEAP MEMORY: %lu bytes", (unsigned long)m_freeHeap.load());

			bool const shouldNotify =
				!wasLowMemory ||
				(m_uptimeSeconds.load() - lastLowMemoryNotifyAt >= LOW_MEMORY_NOTIFY_COOLDOWN_SECONDS);
			if (shouldNotify) {
				flx::core::EventBus::getInstance().publish("system.notify", lowMemoryNotificationTemplate());
				lastLowMemoryNotifyAt = m_uptimeSeconds.load();
			}
			wasLowMemory = true;
		} else {
			wasLowMemory = false;
		}

		if (m_uptimeSeconds % 60 == 0) {
			Log::info(TAG, "Stats - Heap: %lu, PSRAM: %lu, Uptime: %lu s", (unsigned long)m_freeHeap.load(), (unsigned long)m_freePsram.load(), (unsigned long)m_uptimeSeconds.load());
		}
		vTaskDelay(pdMS_TO_TICKS(10000));
	}
}

} // namespace flx::kernel
