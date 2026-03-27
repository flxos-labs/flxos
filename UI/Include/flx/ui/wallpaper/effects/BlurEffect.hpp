#pragma once

#include <flx/ui/wallpaper/EffectPipeline.hpp>
#include <cstdint>
#include <string>

namespace flx::ui::wallpaper::effects {

/**
 * @brief Gaussian blur effect applied via LVGL blur style property.
 *
 * Uses `lv_obj_set_style_blur_radius()` to blur the wallpaper container.
 * The radius parameter (0–20) controls the blur strength.
 */
class BlurEffect : public IEffect {
public:

	/** @param radius  Blur radius in pixels (0 = disabled, max ~20). */
	explicit BlurEffect(uint8_t radius = 4) : m_radius(radius) {}

	void setRadius(uint8_t radius) { m_radius = radius; }
	uint8_t getRadius() const { return m_radius; }

	void process(lv_obj_t* target, uint32_t /*elapsed_ms*/) override {
		if (target == nullptr) {
			return;
		}
		lv_obj_set_style_blur_radius(target, static_cast<int32_t>(m_radius), 0);
	}

	void reset(lv_obj_t* target) override {
		if (target != nullptr) {
			lv_obj_set_style_blur_radius(target, 0, 0);
		}
	}

	std::string getName() const override { return "blur"; }

private:

	uint8_t m_radius;
};

} // namespace flx::ui::wallpaper::effects
