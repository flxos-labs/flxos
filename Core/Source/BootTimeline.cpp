#include <flx/core/BootTimeline.hpp>
#include <flx/core/Logger.hpp>

#include "esp_timer.h"

static constexpr const char* TAG = "BootTimeline";

namespace flx::core {

BootTimeline& BootTimeline::getInstance() {
	static BootTimeline instance;
	return instance;
}

void BootTimeline::record(const std::string& component, const std::string& event,
                          int32_t heapDelta, int64_t durationUs) {
	std::lock_guard<std::mutex> lock(m_mutex);

	Entry entry;
	entry.timestampUs = esp_timer_get_time();
	entry.component = component;
	entry.event = event;
	entry.heapDelta = heapDelta;
	entry.durationUs = durationUs;

	m_entries.push_back(std::move(entry));
}

std::vector<BootTimeline::Entry> BootTimeline::getEntries() const {
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_entries;
}

size_t BootTimeline::getEntryCount() const {
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_entries.size();
}

void BootTimeline::dump() const {
	std::lock_guard<std::mutex> lock(m_mutex);

	Log::info(TAG, "=== Boot Timeline (%zu entries) ===", m_entries.size());

	if (m_entries.empty()) {
		Log::info(TAG, "  (no entries)");
		Log::info(TAG, "==================================");
		return;
	}

	int64_t baseTime = m_entries.front().timestampUs;

	for (size_t i = 0; i < m_entries.size(); i++) {
		const auto& e = m_entries[i];

		// Relative time from first entry, in milliseconds
		int64_t relativeMs = (e.timestampUs - baseTime) / 1000;

		if (e.durationUs > 0) {
			Log::info(TAG, "  [%4lld ms] %-40s %-12s  dur=%lld ms  heap=%+ld B",
			          (long long)relativeMs,
			          e.component.c_str(),
			          e.event.c_str(),
			          (long long)(e.durationUs / 1000),
			          (long)e.heapDelta);
		} else {
			Log::info(TAG, "  [%4lld ms] %-40s %-12s  heap=%+ld B",
			          (long long)relativeMs,
			          e.component.c_str(),
			          e.event.c_str(),
			          (long)e.heapDelta);
		}
	}

	int64_t totalMs = (m_entries.back().timestampUs - baseTime) / 1000;
	Log::info(TAG, "  Total boot span: %lld ms", (long long)totalMs);
	Log::info(TAG, "==================================");
}

int64_t BootTimeline::getTotalBootTimeUs() const {
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_entries.size() < 2) return 0;
	return m_entries.back().timestampUs - m_entries.front().timestampUs;
}

void BootTimeline::clear() {
	std::lock_guard<std::mutex> lock(m_mutex);
	m_entries.clear();
}

} // namespace flx::core
