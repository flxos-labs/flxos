#include "SystemManager.hpp"
#include "../../../hal/display/lv_lgfx_user.hpp"
#include "cJSON.h"
#include "core/apps/AppManager.hpp"
#include "core/common/Logger.hpp"
#include "core/connectivity/ConnectivityManager.hpp"
#include "core/tasks/TaskManager.hpp"
#include "core/tasks/gui/GuiTask.hpp"
#include "core/tasks/resource_monitor/ResourceMonitorTask.hpp"
#include "core/ui/theming/ThemeEngine/ThemeEngine.hpp"
#include "esp_vfs_fat.h"
#include "nvs_flash.h"
#include "src/debugging/sysmon/lv_sysmon.h"
#include <cstring>
#include <stdio.h>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>

static constexpr std::string_view TAG = "SystemManager";

// Internal struct from lv_lovyan_gfx.cpp
#if LV_USE_LOVYAN_GFX
typedef struct {
	LGFX* tft;
} lv_lovyan_gfx_t;
#endif

namespace System {
SystemManager& SystemManager::getInstance() {
	static SystemManager instance;
	return instance;
}

esp_err_t SystemManager::initHardware() {
	Log::info(TAG, "Starting hardware initialization...");
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
		err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		Log::warn(TAG, "NVS flash erase required");
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}
	ESP_ERROR_CHECK(err);

	mount_storage_helper("/system", "system", &m_wl_handle_system, false);
	mount_storage_helper("/data", "data", &m_wl_handle_data, true);

	if (m_wl_handle_system == WL_INVALID_HANDLE) {
		Log::error(TAG, "Failed to mount /system - active SAFE MODE");
		m_isSafeMode = true;
	} else {
		Log::info(TAG, "System storage mounted successfully");
	}

	TaskManager::getInstance().initWatchdog();
	ResourceMonitorTask::getInstance().start();
	return ESP_OK;
}

esp_err_t SystemManager::initGuiState() {

	ConnectivityManager::getInstance().init();

	GuiTask::lock();

	m_save_timer = lv_timer_create([](lv_timer_t* t) {
		SystemManager::getInstance().saveSettings();
		lv_timer_pause(t);
	},
								   2000, nullptr);
	lv_timer_pause(m_save_timer);

	lv_subject_init_int(&m_brightness_subject, 127);
	lv_subject_init_int(&m_theme_subject, (int32_t)ThemeEngine::get_current_theme());
	lv_subject_init_int(&m_rotation_subject, 90);
	lv_subject_init_int(&m_uptime_subject, 0);
	lv_subject_init_int(&m_show_fps_subject, 0);
	lv_subject_init_int(&m_wallpaper_enabled_subject, 0);
	lv_subject_init_int(&m_glass_enabled_subject, 0);
	lv_subject_init_int(&m_transparency_enabled_subject, 1);
	lv_subject_init_int(&m_transparency_enabled_subject, 1);
	lv_subject_init_pointer(&m_wallpaper_path_subject, nullptr);

	// Default Hotspot Settings
	static char default_ssid[] = "ESP32-Hotspot";
	static char default_pass[] = "12345678";
	lv_subject_init_pointer(&m_hotspot_ssid_subject, default_ssid);
	lv_subject_init_pointer(&m_hotspot_password_subject, default_pass);
	lv_subject_init_int(&m_hotspot_channel_subject, 1);
	lv_subject_init_int(&m_hotspot_max_conn_subject, 4);
	lv_subject_init_int(&m_hotspot_hidden_subject, 0); // 0 = visible
	lv_subject_init_int(&m_hotspot_auth_subject, 1); // 1 = WPA2 PSK

	auto add_obs = [](lv_subject_t* s,
					  void (*cb)(lv_observer_t*, lv_subject_t*)) {
		lv_subject_add_observer(s, cb, nullptr);
	};

	add_obs(&m_brightness_subject, [](lv_observer_t*, lv_subject_t* s) {
		int val = lv_subject_get_int(s);
		if (auto d = lv_display_get_default()) {
#if LV_USE_LOVYAN_GFX
			auto dsc = (lv_lovyan_gfx_t*)lv_display_get_driver_data(d);
			if (dsc && dsc->tft)
				dsc->tft->setBrightness((uint8_t)val);
#endif
		}
	});

	add_obs(&m_theme_subject, [](lv_observer_t*, lv_subject_t* s) {
		int theme = lv_subject_get_int(s);
		ThemeEngine::set_theme((ThemeType)theme);
	});

	add_obs(&m_rotation_subject, [](lv_observer_t*, lv_subject_t* s) {
		int rot = lv_subject_get_int(s);
		if (auto d = lv_display_get_default())
			lv_display_set_rotation(
				d, (lv_display_rotation_t)(rot / 90)
			);
	});

	add_obs(&m_show_fps_subject, [](lv_observer_t*, lv_subject_t* s) {
		int show = lv_subject_get_int(s);
#if LV_USE_SYSMON && LV_USE_PERF_MONITOR
		if (show) {
			lv_sysmon_show_performance(lv_display_get_default());
		} else {
			lv_sysmon_hide_performance(lv_display_get_default());
		}
#endif
	});

	loadSettings();

	auto trigger_save_cb = [](lv_observer_t*, lv_subject_t*) {
		SystemManager::getInstance().triggerSave();
	};

	add_obs(&m_brightness_subject, trigger_save_cb);
	add_obs(&m_theme_subject, trigger_save_cb);
	add_obs(&m_rotation_subject, trigger_save_cb);
	add_obs(&m_show_fps_subject, trigger_save_cb);
	add_obs(&m_wallpaper_enabled_subject, trigger_save_cb);
	add_obs(&m_glass_enabled_subject, trigger_save_cb);
	add_obs(&m_transparency_enabled_subject, trigger_save_cb);
	add_obs(&m_transparency_enabled_subject, trigger_save_cb);
	add_obs(&m_wallpaper_path_subject, trigger_save_cb);
	add_obs(&m_hotspot_ssid_subject, trigger_save_cb);
	add_obs(&m_hotspot_password_subject, trigger_save_cb);
	add_obs(&m_hotspot_channel_subject, trigger_save_cb);
	add_obs(&m_hotspot_max_conn_subject, trigger_save_cb);
	add_obs(&m_hotspot_hidden_subject, trigger_save_cb);
	add_obs(&m_hotspot_auth_subject, trigger_save_cb);

	if (auto d = lv_display_get_default()) {
#if LV_USE_LOVYAN_GFX
		auto dsc = (lv_lovyan_gfx_t*)lv_display_get_driver_data(d);
		if (dsc && dsc->tft)
			dsc->tft->setBrightness(
				(uint8_t)lv_subject_get_int(&m_brightness_subject)
			);
#endif
		lv_display_set_rotation(
			d,
			(lv_display_rotation_t)(lv_subject_get_int(&m_rotation_subject) / 90)
		);
	}
	GuiTask::unlock();

	ConnectivityManager::getInstance().startHotspotUsageTimer();

	Apps::AppManager::getInstance().init();

	return ESP_OK;
}

void SystemManager::mount_storage_helper(const char* p, const char* l, wl_handle_t* h, bool f) {
	Log::info(TAG, "Mounting %s...", p);
	esp_vfs_fat_mount_config_t cfg = {
		.format_if_mount_failed = f,
		.max_files = 5,
		.allocation_unit_size = CONFIG_WL_SECTOR_SIZE,
		.disk_status_check_enable = false,
		.use_one_fat = false,
	};
	if (esp_vfs_fat_spiflash_mount_rw_wl(p, l, &cfg, h) != ESP_OK) {
		Log::error(TAG, "FAILED to mount %s", p);
	} else {
		Log::info(TAG, "Mounted %s on partition %s", p, l);
	}
}

void SystemManager::triggerSave() {
	if (m_save_timer) {
		lv_timer_reset(m_save_timer);
		lv_timer_resume(m_save_timer);
	}
}

void SystemManager::loadSettings() {
	FILE* f = fopen("/data/settings.json", "r");
	if (!f) return;

	fseek(f, 0, SEEK_END);
	long len = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (len > 0) {
		char* buf = (char*)malloc(len + 1);
		if (buf) {
			if (fread(buf, 1, len, f) == (size_t)len) {
				buf[len] = 0;
				cJSON* json = cJSON_Parse(buf);
				if (json) {
					cJSON* item;
					if ((item = cJSON_GetObjectItem(json, "brightness")))
						lv_subject_set_int(&m_brightness_subject, item->valueint);
					if ((item = cJSON_GetObjectItem(json, "theme")))
						lv_subject_set_int(&m_theme_subject, item->valueint);
					if ((item = cJSON_GetObjectItem(json, "rotation")))
						lv_subject_set_int(&m_rotation_subject, item->valueint);
					if ((item = cJSON_GetObjectItem(json, "show_fps")))
						lv_subject_set_int(&m_show_fps_subject, item->valueint);
					if ((item = cJSON_GetObjectItem(json, "wp_enabled")))
						lv_subject_set_int(&m_wallpaper_enabled_subject, item->valueint);
					if ((item = cJSON_GetObjectItem(json, "glass_enabled")))
						lv_subject_set_int(&m_glass_enabled_subject, item->valueint);
					if ((item = cJSON_GetObjectItem(json, "transp_enabled")))
						lv_subject_set_int(&m_transparency_enabled_subject, item->valueint);
					if ((item = cJSON_GetObjectItem(json, "wp_path"))) {
						static char path[256];
						strncpy(path, item->valuestring, sizeof(path) - 1);
						lv_subject_set_pointer(&m_wallpaper_path_subject, path);
					}
					if ((item = cJSON_GetObjectItem(json, "hs_ssid"))) {
						static char ssid[33];
						strncpy(ssid, item->valuestring, sizeof(ssid) - 1);
						lv_subject_set_pointer(&m_hotspot_ssid_subject, ssid);
					}
					if ((item = cJSON_GetObjectItem(json, "hs_pass"))) {
						static char pass[65];
						strncpy(pass, item->valuestring, sizeof(pass) - 1);
						lv_subject_set_pointer(&m_hotspot_password_subject, pass);
					}
					if ((item = cJSON_GetObjectItem(json, "hs_chan"))) lv_subject_set_int(&m_hotspot_channel_subject, item->valueint);
					if ((item = cJSON_GetObjectItem(json, "hs_max"))) lv_subject_set_int(&m_hotspot_max_conn_subject, item->valueint);
					if ((item = cJSON_GetObjectItem(json, "hs_hide"))) lv_subject_set_int(&m_hotspot_hidden_subject, item->valueint);
					if ((item = cJSON_GetObjectItem(json, "hs_auth"))) lv_subject_set_int(&m_hotspot_auth_subject, item->valueint);

					cJSON_Delete(json);
				}
			}
			free(buf);
		}
	}
	fclose(f);
	Log::info(TAG, "Settings loaded from /data/settings.json");
}

void SystemManager::saveSettings() {
	cJSON* json = cJSON_CreateObject();
	cJSON_AddNumberToObject(json, "brightness", lv_subject_get_int(&m_brightness_subject));
	cJSON_AddNumberToObject(json, "theme", lv_subject_get_int(&m_theme_subject));
	cJSON_AddNumberToObject(json, "rotation", lv_subject_get_int(&m_rotation_subject));
	cJSON_AddNumberToObject(json, "show_fps", lv_subject_get_int(&m_show_fps_subject));
	cJSON_AddNumberToObject(json, "wp_enabled", lv_subject_get_int(&m_wallpaper_enabled_subject));
	cJSON_AddNumberToObject(json, "glass_enabled", lv_subject_get_int(&m_glass_enabled_subject));
	cJSON_AddNumberToObject(json, "transp_enabled", lv_subject_get_int(&m_transparency_enabled_subject));

	const char* p = (const char*)lv_subject_get_pointer(&m_wallpaper_path_subject);
	cJSON_AddStringToObject(json, "wp_path", p ? p : "");

	const char* ssid = (const char*)lv_subject_get_pointer(&m_hotspot_ssid_subject);
	cJSON_AddStringToObject(json, "hs_ssid", ssid ? ssid : "");
	const char* pass = (const char*)lv_subject_get_pointer(&m_hotspot_password_subject);
	cJSON_AddStringToObject(json, "hs_pass", pass ? pass : "");
	cJSON_AddNumberToObject(json, "hs_chan", lv_subject_get_int(&m_hotspot_channel_subject));
	cJSON_AddNumberToObject(json, "hs_max", lv_subject_get_int(&m_hotspot_max_conn_subject));
	cJSON_AddNumberToObject(json, "hs_hide", lv_subject_get_int(&m_hotspot_hidden_subject));
	cJSON_AddNumberToObject(json, "hs_auth", lv_subject_get_int(&m_hotspot_auth_subject));

	char* str = cJSON_Print(json);
	if (str) {
		FILE* f = fopen("/data/settings.tmp", "w");
		if (f) {
			fprintf(f, "%s", str);
			fsync(fileno(f));
			fclose(f);
			unlink("/data/settings.json");
			rename("/data/settings.tmp", "/data/settings.json");
			Log::info(TAG, "Settings saved to /data/settings.json");
		} else {
			Log::error(TAG, "Failed to open settings file for writing");
		}
		free(str);
	}
	cJSON_Delete(json);
}

} // namespace System
