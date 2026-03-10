#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace flx::core {

/**
 * @brief Records microsecond-precision boot timeline entries for services and apps.
 *
 * Every service start/stop and app launch is recorded as an Entry with
 * component name, event type, heap impact, and duration.  The full
 * timeline can be dumped for diagnostics or queried for total boot time.
 *
 * Thread-safe: all methods can be called from any task/thread.
 *
 * Usage:
 *   BootTimeline::getInstance().record("service:com.flxos.hal", "started", heapDelta, durationUs);
 *   BootTimeline::getInstance().dump();
 */
class BootTimeline {
public:

	struct Entry {
		int64_t timestampUs;   ///< esp_timer_get_time() when this entry was recorded
		std::string component; ///< e.g. "service:com.flxos.hal" or "app:com.flxos.settings"
		std::string event;     ///< e.g. "start", "started", "failed", "stopped"
		int32_t heapDelta;     ///< Heap change in bytes (negative = consumed)
		int64_t durationUs;    ///< Duration of the operation in microseconds (0 if N/A)
	};

	static BootTimeline& getInstance();

	/**
	 * Record a timeline entry.
	 * @param component  Identifier, e.g. "service:com.flxos.hal"
	 * @param event      Event name, e.g. "started", "failed"
	 * @param heapDelta  Heap change in bytes (0 if unknown)
	 * @param durationUs Duration in microseconds (0 if instantaneous)
	 */
	void record(const std::string& component, const std::string& event,
	            int32_t heapDelta = 0, int64_t durationUs = 0);

	/**
	 * Get all recorded entries (copy).
	 */
	std::vector<Entry> getEntries() const;

	/**
	 * Get number of entries recorded.
	 */
	size_t getEntryCount() const;

	/**
	 * Log the full timeline to serial output.
	 */
	void dump() const;

	/**
	 * Total boot time from first recorded entry to last, in microseconds.
	 * Returns 0 if fewer than 2 entries exist.
	 */
	int64_t getTotalBootTimeUs() const;

	/**
	 * Clear all entries (useful for tests or reboot tracking).
	 */
	void clear();

private:

	BootTimeline() = default;
	~BootTimeline() = default;
	BootTimeline(const BootTimeline&) = delete;
	BootTimeline& operator=(const BootTimeline&) = delete;

	std::vector<Entry> m_entries;
	mutable std::mutex m_mutex;
};

} // namespace flx::core
