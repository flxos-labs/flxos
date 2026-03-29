#pragma once

#include <cstddef>
#include <cstdint>

namespace flx::ui::wallpaper {

/**
 * @brief Lightweight frame-rate and CPU-usage monitor for the wallpaper engine.
 *
 * The Desktop calls `recordFrame()` once per render tick.  The monitor
 * accumulates statistics over a rolling window and exposes them via
 * `getMetrics()`.  `getAdaptiveQualityLevel()` returns a quality hint
 * (0 = low, 1 = medium, 2 = high) that the engine can use to downgrade
 * rendering when performance is constrained.
 */
class PerformanceMonitor {
public:

	struct Metrics {
		float current_fps {0.0f};
		float average_fps {0.0f};
		float cpu_usage_percent {0.0f};
		size_t memory_usage_bytes {0};
		uint32_t frame_time_ms {0};
	};

	/** Thresholds used for adaptive quality decisions. */
	static constexpr float TARGET_FPS = 30.0f;
	static constexpr float CPU_WARNING_PCT = 60.0f;
	static constexpr uint32_t WINDOW_MS = 2000;

	/** Record one completed frame with the given elapsed time (ms). */
	void recordFrame(uint32_t frame_time_ms);

	/** Return a snapshot of the current performance metrics. */
	Metrics getMetrics() const;

	/**
	 * Return true when the engine should reduce rendering complexity to
	 * recover frame-rate.
	 */
	bool shouldReduceQuality() const;

	/**
	 * Compute a suggested quality level based on observed performance.
	 * @return 0 = low, 1 = medium, 2 = high
	 */
	int32_t getAdaptiveQualityLevel() const;

	/** Reset all accumulated statistics. */
	void reset();

private:

	static constexpr size_t SAMPLE_COUNT = 32;

	uint32_t m_samples[SAMPLE_COUNT] {};
	size_t m_sample_index {0};
	size_t m_sample_fill {0};
	uint32_t m_window_elapsed_ms {0};
	uint32_t m_window_frame_count {0};
	float m_ema_fps {0.0f};
};

} // namespace flx::ui::wallpaper
