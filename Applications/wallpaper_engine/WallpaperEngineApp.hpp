#pragma once

#include "lvgl.h"
#include <flx/apps/App.hpp>
#include <flx/apps/AppManifest.hpp>
#include <flx/core/EventBus.hpp>
#include <flx/ui/components/FileBrowser.hpp>
#include <memory>
#include <string>

namespace System::Apps {

/**
 * @brief Wallpaper Engine — dedicated user-facing app for controlling wallpapers.
 *
 * The app acts as a UI shell over WallpaperManager.  It does not own
 * rendering or provider lifecycles directly; all engine operations go
 * through WallpaperManager observables and APIs.
 *
 * Pages:
 *  - Dynamic       — placeholder dynamic wallpaper entry point
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
	void onNewIntent(const flx::apps::Intent& intent) override;
	void onStop() override;

private:

	void navigateFromIntent(const flx::apps::Intent& intent);
	void showDynamicPage();
	void hideAllPages();
	void ensureFallbackBanner();
	void handleWallpaperErrorEvent(const flx::core::Bundle& data);
	void showFallbackBanner(const std::string& message);
	void hideFallbackBanner();
	void retryLastWallpaper();
	void openSourceSelector();
	void pollBannerDismissState();

	lv_obj_t* m_container {nullptr};
	lv_obj_t* m_fallbackBanner {nullptr};
	lv_obj_t* m_fallbackLabel {nullptr};
	lv_obj_t* m_retryBtn {nullptr};
	lv_obj_t* m_chooseSourceBtn {nullptr};
	lv_obj_t* m_dynamicPage {nullptr};
	lv_timer_t* m_bannerPollTimer {nullptr};
	std::unique_ptr<flx::ui::FileBrowser> m_wallpaperBrowser;
	flx::core::EventBus::SubscriptionId m_wallpaperErrorSubscriptionId {0};
	std::string m_lastFailedType;
	std::string m_lastFailedSource;
	std::string m_lastReasonCode;
	uint32_t m_lastErrorTickMs {0};
};

} // namespace System::Apps
