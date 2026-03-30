#pragma once

#include <algorithm>
#include <cstdint>
#include <flx/ui/wallpaper/EffectPipeline.hpp>
#include <string>

namespace flx::ui::wallpaper::effects {

/**
 * @brief Brightness/dim effect implemented via LVGL object opacity.
 *
 * A brightness value of 1.0 maps to full opacity (LV_OPA_COVER).
 * Values below 1.0 dim the wallpaper; values above 1.0 are clamped.
 * For an additive brightness boost, a semi-transparent white overlay is
 * placed on top of the wallpaper object.
 */
class BrightnessEffect : public IEffect {
public:

	/**
	 * @param brightness  Normalised brightness multiplier.
	 *                    Range [0.0, 1.0] — 1.0 = full, 0.0 = black.
	 */
	explicit BrightnessEffect(float brightness = 1.0f) : m_brightness(brightness) {}

	void setBrightness(float brightness) {
		m_brightness = std::max(0.0f, std::min(1.0f, brightness));
	}

	float getBrightness() const { return m_brightness; }

	void process(lv_obj_t* target, uint32_t /*elapsed_ms*/) override {
		if (target == nullptr) {
			return;
		}
		auto opa = static_cast<lv_opa_t>(
			static_cast<float>(LV_OPA_COVER) * m_brightness);
		lv_obj_set_style_opa(target, opa, 0);
	}

	void reset(lv_obj_t* target) override {
		if (target != nullptr) {
			lv_obj_set_style_opa(target, LV_OPA_COVER, 0);
		}
	}

	std::string getName() const override { return "brightness"; }

private:

	float m_brightness;
};

} // namespace flx::ui::wallpaper::effects
