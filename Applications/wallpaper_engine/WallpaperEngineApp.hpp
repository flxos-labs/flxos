#pragma once

#include "lvgl.h"
#include "pages/AdaptivePage.hpp"
#include "pages/DynamicPage.hpp"
#include "pages/EffectsPage.hpp"
#include "pages/PresetsPage.hpp"
#include <flx/apps/App.hpp>
#include <flx/apps/AppManifest.hpp>
#include <memory>

namespace System::Apps {

/**
 * @brief Wallpaper Engine — dedicated user-facing app for controlling wallpapers.
 *
 * The app acts as a UI shell over WallpaperManager.  It does not own
 * rendering or provider lifecycles directly; all engine operations go
 * through WallpaperManager observables and APIs.
 *
 * Pages:
 *  - Main menu (list of feature sections)
 *  - Presets       — browse and apply built-in presets
 *  - Effects       — blur, brightness, animation speed, quality
 *  - Dynamic       — generative algorithm selection
 *  - Adaptive      — context-aware wallpaper modes
 */
class WallpaperEngineApp : public flx::apps::App {
public:

	WallpaperEngineApp() = default;
	~WallpaperEngineApp() override = default;

	std::string getPackageName() const override;
	std::string getAppName() const override;
	const void* getIcon() const override;

	static const flx::apps::AppManifest manifest;

	void createUI(void* parent) override;
	void onStop() override;

private:

	void showMainMenu();
	void showPresetsPage();
	void showEffectsPage();
	void showDynamicPage();
	void showAdaptivePage();
	void hideAllPages();

	lv_obj_t* m_container {nullptr};
	lv_obj_t* m_mainMenu {nullptr};

	std::unique_ptr<WallpaperEngine::PresetsPage> m_presetsPage;
	std::unique_ptr<WallpaperEngine::EffectsPage> m_effectsPage;
	std::unique_ptr<WallpaperEngine::DynamicPage> m_dynamicPage;
	std::unique_ptr<WallpaperEngine::AdaptivePage> m_adaptivePage;
};

} // namespace System::Apps
