#pragma once

#include "lvgl.h"
#include <cstdint>
#include <map>
#include <memory>
#include <string>

namespace flx::ui::wallpaper {

/**
 * @brief Interface for individual visual effects applied to a wallpaper object.
 *
 * Concrete effects process an LVGL object per-frame, modifying its visual
 * properties (opacity, blur radius, overlay colour, etc.) to produce the
 * desired result.  All LVGL calls happen on the UI task.
 */
class IEffect {
public:

	virtual ~IEffect() = default;

	/**
	 * Apply the effect to @p target for the current frame.
	 * @param target   LVGL wallpaper container object.
	 * @param elapsed_ms  Milliseconds since the last frame.
	 */
	virtual void process(lv_obj_t* target, uint32_t elapsed_ms) = 0;

	/** Remove all style overrides this effect applied to @p target. */
	virtual void reset(lv_obj_t* target) = 0;

	/** Unique effect identifier string, e.g. "blur". */
	virtual std::string getName() const = 0;
};

/**
 * @brief Ordered pipeline of IEffect instances applied each frame.
 *
 * The Desktop calls processFrame() once per render tick to apply all
 * active effects to the wallpaper container.  Effects are keyed by name
 * so they can be added, updated, or removed individually.
 */
class EffectPipeline {
public:

	/** Add or replace an effect by name. */
	void addEffect(const std::string& name, std::unique_ptr<IEffect> effect) {
		m_effects[name] = std::move(effect);
	}

	/**
	 * Remove an effect by name.
	 * @param target  Object on which the effect's styles should be cleared.
	 */
	void removeEffect(const std::string& name, lv_obj_t* target = nullptr) {
		auto it = m_effects.find(name);
		if (it != m_effects.end()) {
			if (target != nullptr) {
				it->second->reset(target);
			}
			m_effects.erase(it);
		}
	}

	/** Remove all effects, resetting styles on @p target if provided. */
	void clearAll(lv_obj_t* target = nullptr) {
		if (target != nullptr) {
			for (auto& [name, effect]: m_effects) {
				effect->reset(target);
			}
		}
		m_effects.clear();
	}

	/** Apply all effects to @p target for the current frame. */
	void processFrame(lv_obj_t* target, uint32_t elapsed_ms) {
		if (target == nullptr) {
			return;
		}
		for (auto& [name, effect]: m_effects) {
			effect->process(target, elapsed_ms);
		}
	}

	bool hasEffect(const std::string& name) const {
		return m_effects.count(name) > 0;
	}

	bool isEmpty() const { return m_effects.empty(); }

private:

	std::map<std::string, std::unique_ptr<IEffect>> m_effects;
};

} // namespace flx::ui::wallpaper
