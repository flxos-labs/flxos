#pragma once

#include "lvgl.h"
#include <functional>

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

	lv_obj_t* m_container {nullptr};
	std::function<void()> m_onBack;
};

} // namespace System::Apps::WallpaperEngine
