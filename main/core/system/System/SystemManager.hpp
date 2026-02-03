#pragma once

#include "core/common/Observable.hpp"
#include "esp_err.h"
#include "esp_timer.h"
#include "wear_levelling.h"
#include <memory>

#if !CONFIG_FLXOS_HEADLESS_MODE
#include "core/ui/LvglObserverBridge.hpp"
#include "lvgl.h"
#endif

namespace System {
class SystemManager {
public:

	static SystemManager& getInstance();

	esp_err_t initHardware();
	esp_err_t initServices(); // Headless-compatible: connectivity, settings

#if !CONFIG_FLXOS_HEADLESS_MODE
	esp_err_t initGuiState(); // GUI-only: LVGL subjects, observers, themes
#endif

	wl_handle_t getSystemWlHandle() const { return m_wl_handle_system; }
	wl_handle_t getDataWlHandle() const { return m_wl_handle_data; }

	bool isSafeMode() const { return m_isSafeMode; }

	// Headless-compatible settings (Observable-based)
	Observable<int32_t>& getBrightnessObservable() { return m_brightness_subject; }
	Observable<int32_t>& getThemeObservable() { return m_theme_subject; }
	Observable<int32_t>& getRotationObservable() { return m_rotation_subject; }
	Observable<int32_t>& getUptimeObservable() { return m_uptime_subject; }
	Observable<int32_t>& getShowFpsObservable() { return m_show_fps_subject; }
	Observable<int32_t>& getWallpaperEnabledObservable() { return m_wallpaper_enabled_subject; }
	Observable<int32_t>& getGlassEnabledObservable() { return m_glass_enabled_subject; }
	Observable<int32_t>& getTransparencyEnabledObservable() { return m_transparency_enabled_subject; }
	StringObservable& getWallpaperPathObservable() { return m_wallpaper_path_subject; }
	StringObservable& getHotspotSsidObservable() { return m_hotspot_ssid_subject; }
	StringObservable& getHotspotPasswordObservable() { return m_hotspot_password_subject; }
	Observable<int32_t>& getHotspotChannelObservable() { return m_hotspot_channel_subject; }
	Observable<int32_t>& getHotspotMaxConnObservable() { return m_hotspot_max_conn_subject; }
	Observable<int32_t>& getHotspotHiddenObservable() { return m_hotspot_hidden_subject; }
	Observable<int32_t>& getHotspotAuthObservable() { return m_hotspot_auth_subject; }

#if !CONFIG_FLXOS_HEADLESS_MODE
	// GUI-only: LVGL subject accessors (for use with lv_subject_add_observer)
	lv_subject_t& getBrightnessSubject() { return *m_brightness_bridge->getSubject(); }
	lv_subject_t& getThemeSubject() { return *m_theme_bridge->getSubject(); }
	lv_subject_t& getRotationSubject() { return *m_rotation_bridge->getSubject(); }
	lv_subject_t& getUptimeSubject() { return *m_uptime_bridge->getSubject(); }
	lv_subject_t& getShowFpsSubject() { return *m_show_fps_bridge->getSubject(); }
	lv_subject_t& getWallpaperEnabledSubject() { return *m_wallpaper_enabled_bridge->getSubject(); }
	lv_subject_t& getGlassEnabledSubject() { return *m_glass_enabled_bridge->getSubject(); }
	lv_subject_t& getTransparencyEnabledSubject() { return *m_transparency_enabled_bridge->getSubject(); }
	lv_subject_t& getWallpaperPathSubject() { return *m_wallpaper_path_bridge->getSubject(); }
	lv_subject_t& getHotspotSsidSubject() { return *m_hotspot_ssid_bridge->getSubject(); }
	lv_subject_t& getHotspotPasswordSubject() { return *m_hotspot_password_bridge->getSubject(); }
	lv_subject_t& getHotspotChannelSubject() { return *m_hotspot_channel_bridge->getSubject(); }
	lv_subject_t& getHotspotMaxConnSubject() { return *m_hotspot_max_conn_bridge->getSubject(); }
	lv_subject_t& getHotspotHiddenSubject() { return *m_hotspot_hidden_bridge->getSubject(); }
	lv_subject_t& getHotspotAuthSubject() { return *m_hotspot_auth_bridge->getSubject(); }
#endif

private:

	SystemManager() = default;
	~SystemManager() = default;
	SystemManager(const SystemManager&) = delete;
	SystemManager& operator=(const SystemManager&) = delete;

	esp_err_t mountStorage();
	static void mount_storage_helper(const char* path, const char* partition_label, wl_handle_t* wl_handle, bool format_if_failed);

	void loadSettings();
	void saveSettings();
	void triggerSave();

	wl_handle_t m_wl_handle_system = WL_INVALID_HANDLE;
	wl_handle_t m_wl_handle_data = WL_INVALID_HANDLE;
	bool m_isSafeMode = false;

	// Observable-based settings (work in both headless and GUI modes)
	Observable<int32_t> m_brightness_subject {127};
	Observable<int32_t> m_theme_subject {0};
	Observable<int32_t> m_rotation_subject {90};
	Observable<int32_t> m_uptime_subject {0};
	Observable<int32_t> m_show_fps_subject {0};
	Observable<int32_t> m_wallpaper_enabled_subject {0};
	Observable<int32_t> m_glass_enabled_subject {0};
	Observable<int32_t> m_transparency_enabled_subject {1};
	StringObservable m_wallpaper_path_subject {""};
	StringObservable m_hotspot_ssid_subject {"ESP32-Hotspot"};
	StringObservable m_hotspot_password_subject {"12345678"};
	Observable<int32_t> m_hotspot_channel_subject {1};
	Observable<int32_t> m_hotspot_max_conn_subject {4};
	Observable<int32_t> m_hotspot_hidden_subject {0};
	Observable<int32_t> m_hotspot_auth_subject {1};

#if !CONFIG_FLXOS_HEADLESS_MODE
	// LVGL bridges (initialized in initGuiState)
	std::unique_ptr<LvglObserverBridge<int32_t>> m_brightness_bridge;
	std::unique_ptr<LvglObserverBridge<int32_t>> m_theme_bridge;
	std::unique_ptr<LvglObserverBridge<int32_t>> m_rotation_bridge;
	std::unique_ptr<LvglObserverBridge<int32_t>> m_uptime_bridge;
	std::unique_ptr<LvglObserverBridge<int32_t>> m_show_fps_bridge;
	std::unique_ptr<LvglObserverBridge<int32_t>> m_wallpaper_enabled_bridge;
	std::unique_ptr<LvglObserverBridge<int32_t>> m_glass_enabled_bridge;
	std::unique_ptr<LvglObserverBridge<int32_t>> m_transparency_enabled_bridge;
	std::unique_ptr<LvglStringObserverBridge> m_wallpaper_path_bridge;
	std::unique_ptr<LvglStringObserverBridge> m_hotspot_ssid_bridge;
	std::unique_ptr<LvglStringObserverBridge> m_hotspot_password_bridge;
	std::unique_ptr<LvglObserverBridge<int32_t>> m_hotspot_channel_bridge;
	std::unique_ptr<LvglObserverBridge<int32_t>> m_hotspot_max_conn_bridge;
	std::unique_ptr<LvglObserverBridge<int32_t>> m_hotspot_hidden_bridge;
	std::unique_ptr<LvglObserverBridge<int32_t>> m_hotspot_auth_bridge;

	// LVGL timer for debounced saving
	lv_timer_t* m_save_timer = nullptr;
#else
	// Headless: use esp_timer for debounced saving
	esp_timer_handle_t m_save_timer = nullptr;
#endif
};

} // namespace System
