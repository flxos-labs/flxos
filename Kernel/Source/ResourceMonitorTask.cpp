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
	: Task("res_monitor", 8192, 2, tskNO_AFFINITY) {}

ResourceMonitorTask::Stats ResourceMonitorTask::getLatestStats() const {
	return {m_freeHeap.load(), m_minFreeHeap.load(), m_freePsram.load(), m_uptimeSeconds.load()};
}

void ResourceMonitorTask::run(void* /*data*/) {

	setWatchdogTimeout(15000);

	bool wasLowMemory = false;
	uint32_t lastLowMemoryNotifyAt = 0;
	static constexpr uint32_t LOW_MEMORY_NOTIFY_COOLDOWN_SECONDS = 300;
	static constexpr uint32_t WARN_THRESHOLD_WITH_PSRAM_BYTES = 49152;
	static constexpr uint32_t WARN_THRESHOLD_NO_PSRAM_BYTES = 32768;
	static constexpr uint32_t CRIT_THRESHOLD_BYTES = 20480;
	static constexpr uint32_t WARN_LARGEST_BLOCK_BYTES = 24576;
	static constexpr uint32_t CRIT_LARGEST_BLOCK_BYTES = 16384;

	while (true) {
		heartbeat();

		m_freeHeap = esp_get_free_heap_size();
		m_minFreeHeap = esp_get_minimum_free_heap_size();
		m_freePsram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
		m_uptimeSeconds = (uint32_t)(esp_timer_get_time() / 1000000);
		uint32_t const largestBlock = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
		uint32_t const warnThreshold =
			heap_caps_get_total_size(MALLOC_CAP_SPIRAM) > 0 ? WARN_THRESHOLD_WITH_PSRAM_BYTES : WARN_THRESHOLD_NO_PSRAM_BYTES;

		// PowerManager refresh removed to decouple Kernel from System.
		// TODO: Implement self-updating mechanism in PowerManager or use EventBus.

		if (m_freeHeap < CRIT_THRESHOLD_BYTES || largestBlock < CRIT_LARGEST_BLOCK_BYTES) {
			Log::error(TAG,
				"CRITICAL HEAP: free=%lu bytes largest=%lu bytes; requesting app eviction",
				(unsigned long)m_freeHeap.load(),
				(unsigned long)largestBlock);
			flx::core::EventBus::getInstance().publish("system.memory.critical");
			bool const shouldNotify =
				!wasLowMemory ||
				(m_uptimeSeconds.load() - lastLowMemoryNotifyAt >= LOW_MEMORY_NOTIFY_COOLDOWN_SECONDS);
			if (shouldNotify) {
				flx::core::EventBus::getInstance().publish("system.notify", lowMemoryNotificationTemplate());
				lastLowMemoryNotifyAt = m_uptimeSeconds.load();
			}
			wasLowMemory = true;
		} else if (m_freeHeap < warnThreshold || largestBlock < WARN_LARGEST_BLOCK_BYTES) {
			Log::warn(TAG,
				"LOW HEAP MEMORY: free=%lu bytes largest=%lu bytes",
				(unsigned long)m_freeHeap.load(),
				(unsigned long)largestBlock);

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
			float fragRatio = m_freeHeap > 0 ? (1.0f - (static_cast<float>(largestBlock) / static_cast<float>(m_freeHeap.load()))) * 100.0f : 0.0f;
			Log::info(TAG,
				"Stats - Heap: %lu, largest: %lu, frag: %.1f%%, PSRAM: %lu, Uptime: %lu s",
				(unsigned long)m_freeHeap.load(),
				(unsigned long)largestBlock,
				static_cast<double>(fragRatio),
				(unsigned long)m_freePsram.load(),
				(unsigned long)m_uptimeSeconds.load());
		}
		vTaskDelay(pdMS_TO_TICKS(10000));
	}
}

} // namespace flx::kernel
