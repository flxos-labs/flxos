#include <flx/ui/wallpaper/PerformanceMonitor.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace flx::ui::wallpaper {

void PerformanceMonitor::recordFrame(uint32_t frame_time_ms) {
	m_samples[m_sample_index] = frame_time_ms;
	m_sample_index = (m_sample_index + 1) % SAMPLE_COUNT;
	if (m_sample_fill < SAMPLE_COUNT) {
		++m_sample_fill;
	}

	m_window_elapsed_ms += frame_time_ms;
	++m_window_frame_count;

	if (m_window_elapsed_ms >= WINDOW_MS) {
		float const window_fps = (m_window_elapsed_ms > 0U)
			? (static_cast<float>(m_window_frame_count) * 1000.0f) /
				static_cast<float>(m_window_elapsed_ms)
			: 0.0f;
		constexpr float alpha = 0.3f;
		m_ema_fps = (m_ema_fps > 0.0f)
			? (alpha * window_fps) + ((1.0f - alpha) * m_ema_fps)
			: window_fps;
		m_window_elapsed_ms = 0;
		m_window_frame_count = 0;
	}
}

PerformanceMonitor::Metrics PerformanceMonitor::getMetrics() const {
	Metrics m;

	if (m_sample_fill == 0U) {
		return m;
	}

	// Latest frame time
	size_t const latest = (m_sample_index + SAMPLE_COUNT - 1U) % SAMPLE_COUNT;
	m.frame_time_ms = m_samples[latest];
	m.current_fps = (m.frame_time_ms > 0U)
		? 1000.0f / static_cast<float>(m.frame_time_ms)
		: 0.0f;

	// Average over stored samples
	uint64_t sum = 0;
	for (size_t i = 0; i < m_sample_fill; ++i) {
		sum += m_samples[i];
	}
	float const avg_ms = static_cast<float>(sum) / static_cast<float>(m_sample_fill);
	m.average_fps = (avg_ms > 0.0f) ? 1000.0f / avg_ms : 0.0f;

	// CPU usage as a ratio of average frame time to target
	constexpr float target_ms = 1000.0f / TARGET_FPS;
	m.cpu_usage_percent = std::min(100.0f, (avg_ms / target_ms) * 100.0f);

	return m;
}

bool PerformanceMonitor::shouldReduceQuality() const {
	if (m_sample_fill < 4U) {
		return false;
	}
	Metrics const m = getMetrics();
	return (m.average_fps < (TARGET_FPS * 0.8f)) ||
		(m.cpu_usage_percent > CPU_WARNING_PCT);
}

int32_t PerformanceMonitor::getAdaptiveQualityLevel() const {
	if (m_sample_fill == 0U) {
		return 1; // default: medium
	}
	Metrics const m = getMetrics();
	if (m.average_fps < (TARGET_FPS * 0.6f)) {
		return 0; // low
	}
	if (m.average_fps < (TARGET_FPS * 0.9f)) {
		return 1; // medium
	}
	return 2; // high
}

void PerformanceMonitor::reset() {
	m_sample_index = 0;
	m_sample_fill = 0;
	m_window_elapsed_ms = 0;
	m_window_frame_count = 0;
	m_ema_fps = 0.0f;
	for (size_t i = 0; i < SAMPLE_COUNT; ++i) {
		m_samples[i] = 0;
	}
}

} // namespace flx::ui::wallpaper
