#pragma once

#include "lvgl.h"
#include <functional>
#include <string>

namespace System::Apps::WallpaperEngine {

/**
 * @brief Page for selecting and configuring generative (dynamic) wallpapers.
 *
 * Shows a list of available algorithms.  Tapping an algorithm immediately
 * sets it as the active wallpaper via WallpaperManager.
 */
class DynamicPage {
public:

	DynamicPage(lv_obj_t* parent, std::function<void()> onBack);
	~DynamicPage() = default;

	DynamicPage(const DynamicPage&) = delete;
	DynamicPage& operator=(const DynamicPage&) = delete;

	void show();
	void hide();

private:

	void setAlgorithm(const std::string& algorithm);
	void applyDynamicSource() const;
	void syncFromCurrentWallpaper();

	lv_obj_t* m_container {nullptr};
	lv_obj_t* m_speedSlider {nullptr};
	lv_obj_t* m_noiseSlider {nullptr};
	lv_obj_t* m_particlesSlider {nullptr};
	lv_obj_t* m_paletteLabel {nullptr};
	std::string m_algorithm {"plasma"};
	std::string m_palette {"vivid"};
	int32_t m_speed {100};
	int32_t m_particles {96};
	int32_t m_noise {100};
	std::function<void()> m_onBack;
};

} // namespace System::Apps::WallpaperEngine
