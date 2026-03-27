#pragma once

#include "lvgl.h"
#include <flx/ui/LvglObserverBridge.hpp>
#include <functional>
#include <memory>

namespace System::Apps::WallpaperEngine {

/**
 * @brief Page providing controls for visual effects (blur, brightness).
 *
 * The page binds directly to WallpaperManager observables and exposes
 * effect-specific sliders.  Changes are applied immediately through the
 * observable system.
 */
class EffectsPage {
public:

	EffectsPage(lv_obj_t* parent, std::function<void()> onBack);
	~EffectsPage() = default;

	EffectsPage(const EffectsPage&) = delete;
	EffectsPage& operator=(const EffectsPage&) = delete;

	void show();
	void hide();

private:

	lv_obj_t* m_container {nullptr};
	std::function<void()> m_onBack;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_wpEnabledBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_animSpeedBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_qualityBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_cpuUsageBridge;
};

} // namespace System::Apps::WallpaperEngine
