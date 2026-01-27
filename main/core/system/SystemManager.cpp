#include "SystemManager.hpp"
#include "core/apps/AppManager.hpp"
#include "core/connectivity/ConnectivityManager.hpp"
#include "core/tasks/TaskManager.hpp"
#include "core/tasks/gui/GuiTask.hpp"
#include "core/tasks/resource_monitor/ResourceMonitorTask.hpp"
#include "core/ui/theming/ThemeEngine.hpp"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_vfs_fat.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "src/debugging/sysmon/lv_sysmon.h"
#include "src/drivers/display/lovyan_gfx/lv_lgfx_user.hpp"
#include "src/drivers/display/lovyan_gfx/lv_lovyan_gfx.h"

// Internal struct from lv_lovyan_gfx.cpp
#if LV_USE_LOVYAN_GFX
typedef struct {
  LGFX *tft;
} lv_lovyan_gfx_t;
#endif

static const char *TAG = "SystemManager";

namespace System {
SystemManager &SystemManager::getInstance() {
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
  lv_subject_init_int(&m_brightness_subject, 127);
  lv_subject_init_int(&m_theme_subject,
                      (int32_t)ThemeEngine::get_current_theme());
  lv_subject_init_int(&m_rotation_subject, 90);
  lv_subject_init_int(&m_uptime_subject, 0);
  lv_subject_init_int(&m_show_fps_subject, 0);
  lv_subject_init_int(&m_wallpaper_enabled_subject, 0);
  lv_subject_init_int(&m_glass_enabled_subject, 0);

  auto add_obs = [](lv_subject_t *s,
                    void (*cb)(lv_observer_t *, lv_subject_t *)) {
    lv_subject_add_observer(s, cb, nullptr);
  };

  add_obs(&m_brightness_subject, [](lv_observer_t *, lv_subject_t *s) {
    if (auto d = lv_display_get_default()) {
#if LV_USE_LOVYAN_GFX
      auto dsc = (lv_lovyan_gfx_t *)lv_display_get_driver_data(d);
      if (dsc && dsc->tft)
        dsc->tft->setBrightness((uint8_t)lv_subject_get_int(s));
#endif
    }
  });

  add_obs(&m_theme_subject, [](lv_observer_t *, lv_subject_t *s) {
    ThemeEngine::set_theme((ThemeType)lv_subject_get_int(s));
  });

  add_obs(&m_rotation_subject, [](lv_observer_t *, lv_subject_t *s) {
    if (auto d = lv_display_get_default())
      lv_display_set_rotation(
          d, (lv_display_rotation_t)(lv_subject_get_int(s) / 90));
  });

  add_obs(&m_show_fps_subject, [](lv_observer_t *, lv_subject_t *s) {
#if LV_USE_SYSMON && LV_USE_PERF_MONITOR
    if (lv_subject_get_int(s)) {
      lv_sysmon_show_performance(lv_display_get_default());
    } else {
      lv_sysmon_hide_performance(lv_display_get_default());
    }
#endif
  });

  if (auto d = lv_display_get_default()) {
#if LV_USE_LOVYAN_GFX
    auto dsc = (lv_lovyan_gfx_t *)lv_display_get_driver_data(d);
    if (dsc && dsc->tft)
      dsc->tft->setBrightness(
          (uint8_t)lv_subject_get_int(&m_brightness_subject));
#endif
    lv_display_set_rotation(
        d,
        (lv_display_rotation_t)(lv_subject_get_int(&m_rotation_subject) / 90));
  }
  GuiTask::unlock();

  ESP_LOGI(TAG, "GUI State initialized.");

  ConnectivityManager::getInstance().startHotspotUsageTimer();

  ESP_LOGI(TAG, "Initializing AppManager...");
  Apps::AppManager::getInstance().init();
  ESP_LOGI(TAG, "AppManager initialized.");

  return ESP_OK;
}

void SystemManager::mount_storage_helper(const char *p, const char *l,
                                         wl_handle_t *h, bool f) {
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