#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "esp_timer.h"

namespace flx::core {

/**
 * @brief Records every service and app start/stop with microsecond timestamps.
 *
 * Provides a diagnostic timeline of the boot process and runtime
 * lifecycle events. Each entry captures the component, event type,
 * heap impact, and duration.
 *
 * Usage:
 *   BootTimeline::getInstance().record("service:com.flxos.hal", "started", heapDelta, durationUs);
 *   BootTimeline::getInstance().dump();
 */
class BootTimeline {
public:

	struct Entry {
		int64_t timestampUs;    ///< Absolute timestamp (esp_timer_get_time)
		std::string component;  ///< "service:<id>" or "app:<id>"
		std::string event;      ///< "start", "started", "failed", "stopped"
		int32_t heapDelta;      ///< Heap change in bytes (negative = consumed)
		int64_t durationUs;     ///< Duration of the operation in microseconds
	};

	static BootTimeline& getInstance();

	/**
	 * Record a timeline entry.
	 * @param component  Component identifier (e.g. "service:com.flxos.hal")
	 * @param event      Event name (e.g. "started", "failed", "stopped")
	 * @param heapDelta  Heap change from the operation
	 * @param durationUs Duration of the operation in microseconds
	 */
	void record(const std::string& component, const std::string& event,
	            int32_t heapDelta = 0, int64_t durationUs = 0);

	/** Get all recorded entries */
	const std::vector<Entry>& getEntries() const { return m_entries; }

	/** Get number of recorded entries */
	size_t getEntryCount() const { return m_entries.size(); }

	/** Log the full timeline to console */
	void dump() const;

	/**
	 * Total boot time from the first recorded entry to the last.
	 * @return Duration in microseconds, or 0 if fewer than 2 entries.
	 */
	int64_t getTotalBootTimeUs() const;

	/** Clear all recorded entries */
	void clear() { m_entries.clear(); }

private:

	BootTimeline() = default;
	~BootTimeline() = default;
	BootTimeline(const BootTimeline&) = delete;
	BootTimeline& operator=(const BootTimeline&) = delete;

	std::vector<Entry> m_entries;
};

} // namespace flx::core
