#include "SystemManager.hpp"
#include "../../lv_lgfx_user.hpp"
#include "core/apps/AppManager.hpp"
#include "core/connectivity/ConnectivityManager.hpp"
#include "core/tasks/TaskManager.hpp"
#include "core/tasks/gui/GuiTask.hpp"
#include "core/tasks/resource_monitor/ResourceMonitorTask.hpp"
#include "core/ui/theming/ThemeEngine.hpp"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "nvs_flash.h"
#include "src/debugging/sysmon/lv_sysmon.h"
#include "src/drivers/display/lovyan_gfx/lv_lovyan_gfx.h"

// Internal struct from lv_lovyan_gfx.cpp
#if LV_USE_LOVYAN_GFX
typedef struct {
	LGFX* tft;
} lv_lovyan_gfx_t;
#endif

static const char* TAG = "SystemManager";

namespace System {
SystemManager& SystemManager::getInstance() {
	static SystemManager instance;
	return instance;
}

esp_err_t SystemManager::initHardware() {
	ESP_LOGI(TAG, "Initializing Hardware...");
	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
		err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
	}
	ESP_ERROR_CHECK(err);

	mount_storage_helper("/system", "system", &m_wl_handle_system, false);
	mount_storage_helper("/data", "data", &m_wl_handle_data, true);

	if (m_wl_handle_system == WL_INVALID_HANDLE) {
		ESP_LOGE(TAG, "CRITICAL: Failed to mount /system. ENTERING SAFE MODE");
		m_isSafeMode = true;
	}

	TaskManager::getInstance().initWatchdog();
	ResourceMonitorTask::getInstance().start();
	return ESP_OK;
}

esp_err_t SystemManager::initGuiState() {
	ESP_LOGI(TAG, "Initializing GUI State...");

	ConnectivityManager::getInstance().init();

	GuiTask::lock();
	ESP_LOGD(TAG, "Initializing brightness subject (default 127)");
	lv_subject_init_int(&m_brightness_subject, 127);
	ESP_LOGD(TAG, "Initializing theme subject (current: %d)", (int)ThemeEngine::get_current_theme());
	lv_subject_init_int(&m_theme_subject, (int32_t)ThemeEngine::get_current_theme());
	ESP_LOGD(TAG, "Initializing rotation subject (default 90)");
	lv_subject_init_int(&m_rotation_subject, 90);
	ESP_LOGD(TAG, "Initializing uptime subject");
	lv_subject_init_int(&m_uptime_subject, 0);
	ESP_LOGD(TAG, "Initializing show_fps subject");
	lv_subject_init_int(&m_show_fps_subject, 0);
	ESP_LOGD(TAG, "Initializing wallpaper_enabled subject");
	lv_subject_init_int(&m_wallpaper_enabled_subject, 0);
	ESP_LOGD(TAG, "Initializing glass_enabled subject");
	lv_subject_init_int(&m_glass_enabled_subject, 0);
	ESP_LOGD(TAG, "Initializing wallpaper_path subject");
	lv_subject_init_pointer(&m_wallpaper_path_subject, nullptr);

	auto add_obs = [](lv_subject_t* s,
					  void (*cb)(lv_observer_t*, lv_subject_t*)) {
		lv_subject_add_observer(s, cb, nullptr);
	};

	add_obs(&m_brightness_subject, [](lv_observer_t*, lv_subject_t* s) {
		int val = lv_subject_get_int(s);
		ESP_LOGI(TAG, "Brightness observer: Setting brightness to %d", val);
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
		ESP_LOGI(TAG, "Theme observer: Setting theme to %d", theme);
		ThemeEngine::set_theme((ThemeType)theme);
	});

	add_obs(&m_rotation_subject, [](lv_observer_t*, lv_subject_t* s) {
		int rot = lv_subject_get_int(s);
		ESP_LOGI(TAG, "Rotation observer: Setting rotation to %d", rot);
		if (auto d = lv_display_get_default())
			lv_display_set_rotation(
				d, (lv_display_rotation_t)(rot / 90)
			);
	});

	add_obs(&m_show_fps_subject, [](lv_observer_t*, lv_subject_t* s) {
		int show = lv_subject_get_int(s);
		ESP_LOGI(TAG, "FPS Monitor observer: %s", show ? "Showing" : "Hiding");
#if LV_USE_SYSMON && LV_USE_PERF_MONITOR
		if (show) {
			lv_sysmon_show_performance(lv_display_get_default());
		} else {
			lv_sysmon_hide_performance(lv_display_get_default());
		}
#endif
	});

	// Load wallpaper path from NVS
	nvs_handle_t nvs;
	if (nvs_open("storage", NVS_READWRITE, &nvs) == ESP_OK) {
		char path[256];
		size_t sz = sizeof(path);
		ESP_LOGD(TAG, "NVS: Attempting to load wallpaper path");
		if (nvs_get_str(nvs, "wp_path", path, &sz) == ESP_OK) {
			ESP_LOGI(TAG, "NVS: Loaded wallpaper path: %s", path);
			static char saved_path[256];
			strncpy(saved_path, path, sizeof(saved_path) - 1);
			lv_subject_set_pointer(&m_wallpaper_path_subject, saved_path);
		} else {
			ESP_LOGD(TAG, "NVS: No wallpaper path found in storage");
		}
		nvs_close(nvs);
	}

	// Observer to save wallpaper path
	add_obs(&m_wallpaper_path_subject, [](lv_observer_t*, lv_subject_t* s) {
		const char* path = (const char*)lv_subject_get_pointer(s);
		ESP_LOGI(TAG, "Wallpaper path observer: Updating to %s", path ? path : "NULL");
		if (path) {
			nvs_handle_t nvs;
			if (nvs_open("storage", NVS_READWRITE, &nvs) == ESP_OK) {
				esp_err_t err = nvs_set_str(nvs, "wp_path", path);
				if (err == ESP_OK) {
					nvs_commit(nvs);
					ESP_LOGD(TAG, "NVS: Successfully saved wallpaper path");
				} else {
					ESP_LOGE(TAG, "NVS: Failed to save wallpaper path: %s", esp_err_to_name(err));
				}
				nvs_close(nvs);
			}
		}
	});

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

	ESP_LOGI(TAG, "GUI State initialized.");

	ConnectivityManager::getInstance().startHotspotUsageTimer();

	ESP_LOGI(TAG, "Initializing AppManager...");
	Apps::AppManager::getInstance().init();
	ESP_LOGI(TAG, "AppManager initialized.");

	return ESP_OK;
}

void SystemManager::mount_storage_helper(const char* p, const char* l, wl_handle_t* h, bool f) {
	esp_vfs_fat_mount_config_t cfg = {
		.format_if_mount_failed = f,
		.max_files = 5,
		.allocation_unit_size = CONFIG_WL_SECTOR_SIZE,
		.disk_status_check_enable = false,
		.use_one_fat = false,
	};
	if (esp_vfs_fat_spiflash_mount_rw_wl(p, l, &cfg, h) != ESP_OK) {
		ESP_LOGE(TAG, "Failed to mount %s", p);
	} else {
		ESP_LOGI(TAG, "Mounted %s", p);
	}
}

} // namespace System
