#pragma once

#include "lvgl.h"
#include <flx/ui/LvglObserverBridge.hpp>
#include <flx/ui/components/FileBrowser.hpp>
#include <functional>
#include <memory>
#include <string>

namespace System::Apps::WallpaperEngine {

/**
 * @brief Page providing core wallpaper controls and visual effects.
 *
 * This page is the primary settings surface for wallpaper basics:
 * source selection, type switching, animation speed, and effect toggles.
 * Changes are applied immediately through WallpaperManager APIs.
 */
class EffectsPage {
public:

	EffectsPage(lv_obj_t* parent, std::function<void()> onBack);
	~EffectsPage();

	EffectsPage(const EffectsPage&) = delete;
	EffectsPage& operator=(const EffectsPage&) = delete;

	void show();
	void hide();

private:

	lv_obj_t* m_parent {nullptr};
	lv_obj_t* m_container {nullptr};
	std::function<void()> m_onBack;
	flx::ui::FileBrowser* m_fileBrowser {nullptr};
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_wpEnabledBridge;
	std::unique_ptr<flx::ui::LvglStringObserverBridge> m_wpSourceBridge;
	std::unique_ptr<flx::ui::LvglStringObserverBridge> m_wpTypeBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_animSpeedBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_qualityBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_cpuUsageBridge;
};

} // namespace System::Apps::WallpaperEngine
