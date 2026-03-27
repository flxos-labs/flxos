#pragma once

#include "lvgl.h"
#include <flx/ui/LvglObserverBridge.hpp>
#include <functional>
#include <memory>
#include <string>

namespace System::Apps::WallpaperEngine {

/**
 * @brief A labelled slider row used in the Effects page.
 *
 * Creates a horizontally laid-out row containing a label, a slider bound
 * to the provided subject, and a live numeric value label.  The widget is
 * added as a child of @p parent; LVGL owns the underlying objects.
 */
class EffectSliderRow {
public:

	/**
	 * @param parent   Container object in which to create the row.
	 * @param label    Display label shown to the left of the slider.
	 * @param subject  LVGL integer subject to bind the slider to.
	 * @param min_val  Minimum slider value.
	 * @param max_val  Maximum slider value.
	 */
	EffectSliderRow(
		lv_obj_t* parent,
		const char* label,
		lv_subject_t* subject,
		int32_t min_val,
		int32_t max_val);

	~EffectSliderRow() = default;

	EffectSliderRow(const EffectSliderRow&) = delete;
	EffectSliderRow& operator=(const EffectSliderRow&) = delete;
};

} // namespace System::Apps::WallpaperEngine
