#pragma once

#include <flx/ui/LvglObserverBridge.hpp>
#include <flx/ui/desktop/modules/dock/Dock.hpp>
#include <flx/ui/desktop/modules/floating_notifications/FloatingNotifications.hpp>
#include <flx/ui/desktop/modules/launcher/Launcher.hpp>
#include <flx/ui/desktop/modules/notification_panel/NotificationPanel.hpp>
#include <flx/ui/desktop/modules/quick_access_panel/QuickAccessPanel.hpp>
#include <flx/ui/desktop/modules/status_bar/StatusBar.hpp>
#include <flx/ui/desktop/modules/swipe_manager/SwipeManager.hpp>
#include <flx/ui/wallpaper/IWallpaperProvider.hpp>
#include <lvgl.h>
#include <memory>
#include <string>
#include <vector>

namespace UI {

class Desktop {
public:

	static Desktop& getInstance();
	Desktop();
	~Desktop();

	void init();
	void onFrame(uint32_t delta_ms);

	// These now delegate to WM
	void openApp(const std::string& packageName);
	void closeApp(const std::string& packageName);

private:

	lv_obj_t* m_screen {};
	lv_obj_t* m_wallpaper {};
	lv_obj_t* m_wallpaper_icon {};
	lv_obj_t* m_window_container {};
	std::unique_ptr<UI::Modules::StatusBar> m_statusBarModule {};
	lv_obj_t* m_status_bar {};
	std::unique_ptr<UI::Modules::FloatingNotifications> m_floatingNotificationsModule {};
	std::unique_ptr<UI::Modules::Dock> m_dockModule {};
	lv_obj_t* m_dock {};
	std::unique_ptr<UI::Modules::Launcher> m_launcherModule {};
	lv_obj_t* m_launcher {};
	std::unique_ptr<UI::Modules::QuickAccessPanel> m_quickAccessPanelModule {};
	lv_obj_t* m_quick_access_panel {};
	std::unique_ptr<UI::Modules::NotificationPanel> m_notificationPanelModule {};
	lv_obj_t* m_notification_panel {};
	lv_obj_t* m_notification_list {};
	lv_obj_t* m_clear_all_btn {};
	lv_obj_t* m_greetings {};
	lv_obj_t* m_wallpaper_perf_overlay {};
	lv_obj_t* m_app_container {};
	std::unique_ptr<UI::Modules::SwipeManager> m_swipeManagerModule {};
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_rotationObserver;
	std::unique_ptr<flx::ui::wallpaper::IWallpaperProvider> m_wallpaperProvider;
	std::string m_wallpaperProviderType {""};
	std::string m_wallpaperProviderSource {""};
	int32_t m_wallpaperProviderSpeed {-1};
	std::string m_lastWallpaperFailureKey {""};
	uint32_t m_providerBaselineHeapBytes {0};
	uint32_t m_providerPerfWindowMs {0};
	uint32_t m_providerPerfFrameCount {0};
	std::string m_providerPerfKey {""};
	uint32_t m_providerBenchmarkWindowMs {0};
	uint32_t m_providerBenchmarkFrameCount {0};
	uint64_t m_providerBenchmarkTotalFrameMs {0};
	uint32_t m_providerBenchmarkMaxFrameMs {0};
	std::vector<uint32_t> m_providerBenchmarkFrameSamples {};
	uint32_t m_providerBenchmarkBaselineHeapBytes {0};
	std::string m_providerBenchmarkKey {""};
	std::string m_providerBenchmarkPath {"/data/wallpaper_benchmark.csv"};
	bool m_providerBenchmarkHeaderWritten {false};
	bool m_providerBenchmarkWriteFailed {false};
	uint32_t m_overlayWindowMs {0};
	uint32_t m_overlayFrameCount {0};
	uint64_t m_overlayTotalFrameMs {0};
	uint32_t m_overlayMaxFrameMs {0};
	bool m_lastBenchmarkEnabled {false};

	void update_notification_list();
	void realign_panels();
	void configure_panel_style(lv_obj_t* panel);
	void setWallpaperPerfOverlayVisible(bool visible);
	void updateWallpaperPerfOverlay(const std::string& type, float fps, float avgFrameMs, uint32_t maxFrameMs, uint32_t extraHeapBytes);
	void syncWallpaperProvider(uint32_t delta_ms);
	void handleWallpaperProviderFailure(const std::string& requestedType, const std::string& source, const std::string& error);
	void evaluateWallpaperAcceptanceGate();

	void on_start_click();
	void on_up_click();
	static void on_app_click(lv_event_t* e);
};

} // namespace UI
