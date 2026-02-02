#pragma once

#include "esp_err.h"
#include "lvgl.h"
#include "wear_levelling.h"

namespace System {
class SystemManager {
public:

	static SystemManager& getInstance();

	esp_err_t initHardware();
	esp_err_t initGuiState();

	wl_handle_t getSystemWlHandle() const { return m_wl_handle_system; }
	wl_handle_t getDataWlHandle() const { return m_wl_handle_data; }

	bool isSafeMode() const { return m_isSafeMode; }

	lv_subject_t& getBrightnessSubject() { return m_brightness_subject; }
	lv_subject_t& getThemeSubject() { return m_theme_subject; }
	lv_subject_t& getRotationSubject() { return m_rotation_subject; }
	lv_subject_t& getUptimeSubject() { return m_uptime_subject; }
	lv_subject_t& getShowFpsSubject() { return m_show_fps_subject; }
	lv_subject_t& getWallpaperEnabledSubject() {
		return m_wallpaper_enabled_subject;
	}
	lv_subject_t& getGlassEnabledSubject() { return m_glass_enabled_subject; }
	lv_subject_t& getTransparencyEnabledSubject() {
		return m_transparency_enabled_subject;
	}
	lv_subject_t& getWallpaperPathSubject() { return m_wallpaper_path_subject; }
	lv_subject_t& getHotspotSsidSubject() { return m_hotspot_ssid_subject; }
	lv_subject_t& getHotspotPasswordSubject() { return m_hotspot_password_subject; }
	lv_subject_t& getHotspotChannelSubject() { return m_hotspot_channel_subject; }
	lv_subject_t& getHotspotMaxConnSubject() { return m_hotspot_max_conn_subject; }
	lv_subject_t& getHotspotHiddenSubject() { return m_hotspot_hidden_subject; }
	lv_subject_t& getHotspotAuthSubject() { return m_hotspot_auth_subject; }

private:

	SystemManager() = default;
	~SystemManager() = default;
	SystemManager(const SystemManager&) = delete;
	SystemManager& operator=(const SystemManager&) = delete;

	esp_err_t mountStorage();
	static void mount_storage_helper(const char* path, const char* partition_label, wl_handle_t* wl_handle, bool format_if_failed);

	wl_handle_t m_wl_handle_system = WL_INVALID_HANDLE;
	wl_handle_t m_wl_handle_data = WL_INVALID_HANDLE;
	bool m_isSafeMode = false;

	lv_subject_t m_brightness_subject;
	lv_subject_t m_theme_subject;
	lv_subject_t m_rotation_subject;
	lv_subject_t m_uptime_subject;
	lv_subject_t m_show_fps_subject;
	lv_subject_t m_wallpaper_enabled_subject;
	lv_subject_t m_glass_enabled_subject;
	lv_subject_t m_transparency_enabled_subject;
	lv_subject_t m_wallpaper_path_subject;
	lv_subject_t m_hotspot_ssid_subject;
	lv_subject_t m_hotspot_password_subject;
	lv_subject_t m_hotspot_channel_subject;
	lv_subject_t m_hotspot_max_conn_subject;
	lv_subject_t m_hotspot_hidden_subject;
	lv_subject_t m_hotspot_auth_subject;

	void loadSettings();
	void saveSettings();
	void triggerSave();
	lv_timer_t* m_save_timer = nullptr;
};

} // namespace System
