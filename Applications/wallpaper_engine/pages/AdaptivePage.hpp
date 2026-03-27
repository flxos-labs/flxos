#pragma once

#include "lvgl.h"
#include <flx/ui/LvglObserverBridge.hpp>
#include <functional>
#include <memory>

namespace System::Apps::WallpaperEngine {

/**
 * @brief Page for configuring context-aware (adaptive) wallpaper behaviour.
 *
 * In the current implementation the adaptive feature description is shown
 * informatively.  The page allows the user to enable/disable the wallpaper
 * and select a "time of day" mode which will switch between preset algorithms
 * based on the current hour.
 */
class AdaptivePage {
public:

	AdaptivePage(lv_obj_t* parent, std::function<void()> onBack);
	~AdaptivePage() = default;

	AdaptivePage(const AdaptivePage&) = delete;
	AdaptivePage& operator=(const AdaptivePage&) = delete;

	void show();
	void hide();

private:

	lv_obj_t* m_container {nullptr};
	std::function<void()> m_onBack;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_wpEnabledBridge;
};

} // namespace System::Apps::WallpaperEngine
