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
	Entry entry;
	entry.timestampUs = esp_timer_get_time();
	entry.component = component;
	entry.event = event;
	entry.heapDelta = heapDelta;
	entry.durationUs = durationUs;
	m_entries.push_back(std::move(entry));
}

void BootTimeline::dump() const {
	if (m_entries.empty()) {
		Log::info(TAG, "Boot timeline: (empty)");
		return;
	}

	int64_t bootTimeUs = getTotalBootTimeUs();
	Log::info(TAG, "=== Boot Timeline (%zu entries, total %lld ms) ===",
	          m_entries.size(), (long long)(bootTimeUs / 1000));

	int64_t baseUs = m_entries.front().timestampUs;

	for (size_t i = 0; i < m_entries.size(); ++i) {
		const auto& e = m_entries[i];
		int64_t relativeMs = (e.timestampUs - baseUs) / 1000;
		int64_t durationMs = e.durationUs / 1000;

		if (e.durationUs > 0) {
			Log::info(TAG, "  [+%lld ms] %s %s (%lld ms, heap %+ld B)",
			          (long long)relativeMs,
			          e.component.c_str(),
			          e.event.c_str(),
			          (long long)durationMs,
			          (long)e.heapDelta);
		} else {
			Log::info(TAG, "  [+%lld ms] %s %s",
			          (long long)relativeMs,
			          e.component.c_str(),
			          e.event.c_str());
		}
	}

	Log::info(TAG, "=== End Boot Timeline ===");
}

int64_t BootTimeline::getTotalBootTimeUs() const {
	if (m_entries.size() < 2) return 0;
	return m_entries.back().timestampUs - m_entries.front().timestampUs;
}

} // namespace flx::core
