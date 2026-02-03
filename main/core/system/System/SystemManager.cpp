#include "SystemManager.hpp"
#include "cJSON.h"
#include "core/common/Logger.hpp"
#include "core/connectivity/ConnectivityManager.hpp"
#include "core/tasks/TaskManager.hpp"
#include "core/tasks/resource_monitor/ResourceMonitorTask.hpp"
#include "esp_timer.h"
#include "esp_vfs_fat.h"
#include "nvs_flash.h"
#include <cstring>
#include <memory>
#include <stdio.h>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>

#if !CONFIG_FLXOS_HEADLESS_MODE
#include "../../../hal/display/lv_lgfx_user.hpp"
#include "core/apps/AppManager.hpp"
#include "core/tasks/gui/GuiTask.hpp"
#include "core/ui/theming/ThemeEngine/ThemeEngine.hpp"
#include "src/debugging/sysmon/lv_sysmon.h"

// Internal struct from lv_lovyan_gfx.cpp
#if LV_USE_LOVYAN_GFX
typedef struct {
	LGFX* tft;
} lv_lovyan_gfx_t;
#endif
#endif

static constexpr std::string_view TAG = "SystemManager";

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

esp_err_t SystemManager::initServices() {
	Log::info(TAG, "Initializing services...");

	// Load settings from file
	loadSettings();

	// Initialize connectivity (WiFi, Hotspot, Bluetooth)
	ConnectivityManager::getInstance().init();

#if CONFIG_FLXOS_HEADLESS_MODE
	// In headless mode, use esp_timer for debounced saving
	esp_timer_create_args_t timer_args = {
		.callback = [](void* arg) {
			SystemManager::getInstance().saveSettings();
		},
		.arg = nullptr,
		.dispatch_method = ESP_TIMER_TASK,
		.name = "settings_save",
		.skip_unhandled_events = true,
	};
	esp_timer_create(&timer_args, &m_save_timer);

	// Subscribe to setting changes to trigger save
	auto trigger_save = [](const int32_t&) {
		SystemManager::getInstance().triggerSave();
	};
	auto trigger_save_str = [](const char*) {
		SystemManager::getInstance().triggerSave();
	};

	m_brightness_subject.subscribe(trigger_save);
	m_theme_subject.subscribe(trigger_save);
	m_rotation_subject.subscribe(trigger_save);
	m_show_fps_subject.subscribe(trigger_save);
	m_wallpaper_enabled_subject.subscribe(trigger_save);
	m_glass_enabled_subject.subscribe(trigger_save);
	m_transparency_enabled_subject.subscribe(trigger_save);
	m_wallpaper_path_subject.subscribe(trigger_save_str);
	m_hotspot_ssid_subject.subscribe(trigger_save_str);
	m_hotspot_password_subject.subscribe(trigger_save_str);
	m_hotspot_channel_subject.subscribe(trigger_save);
	m_hotspot_max_conn_subject.subscribe(trigger_save);
	m_hotspot_hidden_subject.subscribe(trigger_save);
	m_hotspot_auth_subject.subscribe(trigger_save);

	// Start hotspot usage timer
	ConnectivityManager::getInstance().startHotspotUsageTimer();
#endif

	Log::info(TAG, "Services initialized");
	return ESP_OK;
}

#if !CONFIG_FLXOS_HEADLESS_MODE
esp_err_t SystemManager::initGuiState() {
	GuiTask::lock();

	// Create LVGL bridges for all settings
	m_brightness_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_brightness_subject);
	m_theme_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_theme_subject);
	m_rotation_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_rotation_subject);
	m_uptime_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_uptime_subject);
	m_show_fps_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_show_fps_subject);
	m_wallpaper_enabled_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_wallpaper_enabled_subject);
	m_glass_enabled_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_glass_enabled_subject);
	m_transparency_enabled_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_transparency_enabled_subject);
	m_wallpaper_path_bridge = std::make_unique<LvglStringObserverBridge>(m_wallpaper_path_subject);
	m_hotspot_ssid_bridge = std::make_unique<LvglStringObserverBridge>(m_hotspot_ssid_subject);
	m_hotspot_password_bridge = std::make_unique<LvglStringObserverBridge>(m_hotspot_password_subject);
	m_hotspot_channel_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_hotspot_channel_subject);
	m_hotspot_max_conn_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_hotspot_max_conn_subject);
	m_hotspot_hidden_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_hotspot_hidden_subject);
	m_hotspot_auth_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_hotspot_auth_subject);

	// Initialize connectivity LVGL bridges
	ConnectivityManager::getInstance().initGuiBridges();

	m_save_timer = lv_timer_create([](lv_timer_t* t) {
		SystemManager::getInstance().saveSettings();
		lv_timer_pause(t);
	},
								   2000, nullptr);
	lv_timer_pause(m_save_timer);

	// Set up LVGL observers for brightness changes
	lv_subject_add_observer(m_brightness_bridge->getSubject(), [](lv_observer_t*, lv_subject_t* s) {
		int val = lv_subject_get_int(s);
		if (auto d = lv_display_get_default()) {
#if LV_USE_LOVYAN_GFX
			auto dsc = (lv_lovyan_gfx_t*)lv_display_get_driver_data(d);
			if (dsc && dsc->tft)
				dsc->tft->setBrightness((uint8_t)val);
#endif
		} }, nullptr);

	lv_subject_add_observer(m_theme_bridge->getSubject(), [](lv_observer_t*, lv_subject_t* s) {
		int theme = lv_subject_get_int(s);
		ThemeEngine::set_theme((ThemeType)theme); }, nullptr);

	lv_subject_add_observer(m_rotation_bridge->getSubject(), [](lv_observer_t*, lv_subject_t* s) {
		int rot = lv_subject_get_int(s);
		if (auto d = lv_display_get_default())
			lv_display_set_rotation(d, (lv_display_rotation_t)(rot / 90)); }, nullptr);

	lv_subject_add_observer(m_show_fps_bridge->getSubject(), [](lv_observer_t*, lv_subject_t* s) {
		int show = lv_subject_get_int(s);
#if LV_USE_SYSMON && LV_USE_PERF_MONITOR
		if (show) {
			lv_sysmon_show_performance(lv_display_get_default());
		} else {
			lv_sysmon_hide_performance(lv_display_get_default());
		}
#endif
	},
							nullptr);

	// Subscribe to save trigger via lv_subject observers
	auto trigger_save_cb = [](lv_observer_t*, lv_subject_t*) {
		SystemManager::getInstance().triggerSave();
	};

	lv_subject_add_observer(m_brightness_bridge->getSubject(), trigger_save_cb, nullptr);
	lv_subject_add_observer(m_theme_bridge->getSubject(), trigger_save_cb, nullptr);
	lv_subject_add_observer(m_rotation_bridge->getSubject(), trigger_save_cb, nullptr);
	lv_subject_add_observer(m_show_fps_bridge->getSubject(), trigger_save_cb, nullptr);
	lv_subject_add_observer(m_wallpaper_enabled_bridge->getSubject(), trigger_save_cb, nullptr);
	lv_subject_add_observer(m_glass_enabled_bridge->getSubject(), trigger_save_cb, nullptr);
	lv_subject_add_observer(m_transparency_enabled_bridge->getSubject(), trigger_save_cb, nullptr);
	lv_subject_add_observer(m_wallpaper_path_bridge->getSubject(), trigger_save_cb, nullptr);
	lv_subject_add_observer(m_hotspot_ssid_bridge->getSubject(), trigger_save_cb, nullptr);
	lv_subject_add_observer(m_hotspot_password_bridge->getSubject(), trigger_save_cb, nullptr);
	lv_subject_add_observer(m_hotspot_channel_bridge->getSubject(), trigger_save_cb, nullptr);
	lv_subject_add_observer(m_hotspot_max_conn_bridge->getSubject(), trigger_save_cb, nullptr);
	lv_subject_add_observer(m_hotspot_hidden_bridge->getSubject(), trigger_save_cb, nullptr);
	lv_subject_add_observer(m_hotspot_auth_bridge->getSubject(), trigger_save_cb, nullptr);

	// Apply initial values
	if (auto d = lv_display_get_default()) {
#if LV_USE_LOVYAN_GFX
		auto dsc = (lv_lovyan_gfx_t*)lv_display_get_driver_data(d);
		if (dsc && dsc->tft)
			dsc->tft->setBrightness((uint8_t)m_brightness_subject.get());
#endif
		lv_display_set_rotation(
			d,
			(lv_display_rotation_t)(m_rotation_subject.get() / 90)
		);
	}
	GuiTask::unlock();

	ConnectivityManager::getInstance().startHotspotUsageTimer();

	Apps::AppManager::getInstance().init();

	return ESP_OK;
}
#endif

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
#if !CONFIG_FLXOS_HEADLESS_MODE
	if (m_save_timer) {
		lv_timer_reset(m_save_timer);
		lv_timer_resume(m_save_timer);
	}
#else
	if (m_save_timer) {
		esp_timer_stop(m_save_timer);
		esp_timer_start_once(m_save_timer, 2000000); // 2 seconds
	}
#endif
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
						m_brightness_subject.set(item->valueint);
					if ((item = cJSON_GetObjectItem(json, "theme")))
						m_theme_subject.set(item->valueint);
					if ((item = cJSON_GetObjectItem(json, "rotation")))
						m_rotation_subject.set(item->valueint);
					if ((item = cJSON_GetObjectItem(json, "show_fps")))
						m_show_fps_subject.set(item->valueint);
					if ((item = cJSON_GetObjectItem(json, "wp_enabled")))
						m_wallpaper_enabled_subject.set(item->valueint);
					if ((item = cJSON_GetObjectItem(json, "glass_enabled")))
						m_glass_enabled_subject.set(item->valueint);
					if ((item = cJSON_GetObjectItem(json, "transp_enabled")))
						m_transparency_enabled_subject.set(item->valueint);
					if ((item = cJSON_GetObjectItem(json, "wp_path")))
						m_wallpaper_path_subject.set(item->valuestring);
					if ((item = cJSON_GetObjectItem(json, "hs_ssid")))
						m_hotspot_ssid_subject.set(item->valuestring);
					if ((item = cJSON_GetObjectItem(json, "hs_pass")))
						m_hotspot_password_subject.set(item->valuestring);
					if ((item = cJSON_GetObjectItem(json, "hs_chan")))
						m_hotspot_channel_subject.set(item->valueint);
					if ((item = cJSON_GetObjectItem(json, "hs_max")))
						m_hotspot_max_conn_subject.set(item->valueint);
					if ((item = cJSON_GetObjectItem(json, "hs_hide")))
						m_hotspot_hidden_subject.set(item->valueint);
					if ((item = cJSON_GetObjectItem(json, "hs_auth")))
						m_hotspot_auth_subject.set(item->valueint);

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
	cJSON_AddNumberToObject(json, "brightness", m_brightness_subject.get());
	cJSON_AddNumberToObject(json, "theme", m_theme_subject.get());
	cJSON_AddNumberToObject(json, "rotation", m_rotation_subject.get());
	cJSON_AddNumberToObject(json, "show_fps", m_show_fps_subject.get());
	cJSON_AddNumberToObject(json, "wp_enabled", m_wallpaper_enabled_subject.get());
	cJSON_AddNumberToObject(json, "glass_enabled", m_glass_enabled_subject.get());
	cJSON_AddNumberToObject(json, "transp_enabled", m_transparency_enabled_subject.get());

	const char* p = m_wallpaper_path_subject.get();
	cJSON_AddStringToObject(json, "wp_path", p ? p : "");

	const char* ssid = m_hotspot_ssid_subject.get();
	cJSON_AddStringToObject(json, "hs_ssid", ssid ? ssid : "");
	const char* pass = m_hotspot_password_subject.get();
	cJSON_AddStringToObject(json, "hs_pass", pass ? pass : "");
	cJSON_AddNumberToObject(json, "hs_chan", m_hotspot_channel_subject.get());
	cJSON_AddNumberToObject(json, "hs_max", m_hotspot_max_conn_subject.get());
	cJSON_AddNumberToObject(json, "hs_hide", m_hotspot_hidden_subject.get());
	cJSON_AddNumberToObject(json, "hs_auth", m_hotspot_auth_subject.get());

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
