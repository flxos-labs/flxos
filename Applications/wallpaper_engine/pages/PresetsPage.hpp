#pragma once

#include "lvgl.h"
#include "../widgets/WallpaperPreviewCard.hpp"
#include <flx/ui/wallpaper/PresetLibrary.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace System::Apps::WallpaperEngine {

/**
 * @brief Page showing all available wallpaper presets in a scrollable list.
 *
 * Each preset is displayed with its name, description, and an Apply button.
 * Tapping Apply calls WallpaperManager::setWallpaper() with the preset's
 * source and type.
 */
class PresetsPage {
public:

	PresetsPage(lv_obj_t* parent, std::function<void()> onBack);
	~PresetsPage() = default;

	PresetsPage(const PresetsPage&) = delete;
	PresetsPage& operator=(const PresetsPage&) = delete;

	void show();
	void hide();

private:

	void applyPreset(const std::string& id);

	lv_obj_t* m_container {nullptr};
	std::function<void()> m_onBack;
	std::vector<std::unique_ptr<WallpaperPreviewCard>> m_cards;
};

} // namespace System::Apps::WallpaperEngine
