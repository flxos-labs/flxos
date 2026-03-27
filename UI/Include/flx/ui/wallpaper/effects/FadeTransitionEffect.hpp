#pragma once

#include <flx/ui/wallpaper/EffectPipeline.hpp>
#include <cstdint>
#include <string>

namespace flx::ui::wallpaper::effects {

/**
 * @brief Animated fade-in / fade-out transition effect.
 *
 * When `startFade()` is called the effect animates the wallpaper container
 * opacity from the current value to the target value over `duration_ms`
 * milliseconds.  The animation is driven by tracking accumulated time in
 * `process()` so it runs on the UI task without a separate LVGL animation
 * object.
 */
class FadeTransitionEffect : public IEffect {
public:

	/** @param duration_ms  Total fade duration in milliseconds. */
	explicit FadeTransitionEffect(uint32_t duration_ms = 500)
		: m_duration_ms(duration_ms) {}

	/** Begin fading from @p from_opa to @p to_opa over the configured duration. */
	void startFade(lv_opa_t from_opa, lv_opa_t to_opa) {
		m_from_opa = from_opa;
		m_to_opa = to_opa;
		m_elapsed_ms = 0;
		m_active = true;
	}

	/** @return true if a fade animation is currently in progress. */
	bool isActive() const { return m_active; }

	void setDuration(uint32_t duration_ms) { m_duration_ms = duration_ms; }

	void process(lv_obj_t* target, uint32_t elapsed_ms) override {
		if (!m_active || target == nullptr) {
			return;
		}
		m_elapsed_ms += elapsed_ms;
		float progress = (m_duration_ms > 0U)
			? static_cast<float>(m_elapsed_ms) / static_cast<float>(m_duration_ms)
			: 1.0f;
		if (progress >= 1.0f) {
			progress = 1.0f;
			m_active = false;
		}
		auto opa = static_cast<lv_opa_t>(
			static_cast<float>(m_from_opa) +
			(static_cast<float>(m_to_opa) - static_cast<float>(m_from_opa)) * progress);
		lv_obj_set_style_opa(target, opa, 0);
	}

	void reset(lv_obj_t* target) override {
		m_active = false;
		m_elapsed_ms = 0;
		if (target != nullptr) {
			lv_obj_set_style_opa(target, LV_OPA_COVER, 0);
		}
	}

	std::string getName() const override { return "fade_transition"; }

private:

	uint32_t m_duration_ms;
	uint32_t m_elapsed_ms {0};
	lv_opa_t m_from_opa {LV_OPA_COVER};
	lv_opa_t m_to_opa {LV_OPA_COVER};
	bool m_active {false};
};

} // namespace flx::ui::wallpaper::effects
