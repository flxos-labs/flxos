#pragma once

#include "lvgl.h"
#include <flx/ui/wallpaper/PresetLibrary.hpp>
#include <functional>
#include <string>

namespace System::Apps::WallpaperEngine {

/**
 * @brief A card widget displaying a preset's name, type badge, and an Apply button.
 *
 * The card is created as a child of @p parent and calls @p onApply when the
 * Apply button is tapped.  Ownership of the underlying LVGL object remains
 * with the parent.  Callback data is allocated on the heap and freed via an
 * LV_EVENT_DELETE handler on the Apply button, so no dangling pointers occur
 * if the button is deleted before the C++ object.
 */
class WallpaperPreviewCard {
public:

	WallpaperPreviewCard(
		lv_obj_t* parent,
		const flx::ui::wallpaper::WallpaperPreset& preset,
		std::function<void(const std::string& id)> onApply);

	~WallpaperPreviewCard() = default;

	WallpaperPreviewCard(const WallpaperPreviewCard&) = delete;
	WallpaperPreviewCard& operator=(const WallpaperPreviewCard&) = delete;

private:

	struct CallbackData {
		std::string preset_id;
		std::function<void(const std::string&)> on_apply;
	};
};

} // namespace System::Apps::WallpaperEngine
