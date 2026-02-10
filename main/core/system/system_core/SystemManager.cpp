#include "SystemManager.hpp"
#include "core/common/Logger.hpp"
#include "core/connectivity/ConnectivityManager.hpp"
#include "core/system/display/DisplayManager.hpp"
#include "core/system/power/PowerManager.hpp"
#include "core/system/settings/SettingsManager.hpp"
#include "core/system/theme/ThemeManager.hpp"
#include "core/system/time/TimeManager.hpp"
#include "core/tasks/TaskManager.hpp"
#include "core/tasks/resource_monitor/ResourceMonitorTask.hpp"
#include "esp_err.h"
#include "esp_vfs_fat.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "wear_levelling.h"
#if defined(CONFIG_FLXOS_SD_CARD_ENABLED)
#include "core/services/storage/SdCardService.hpp"
#endif
#include <cstring>
#include <memory>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>

#if !CONFIG_FLXOS_HEADLESS_MODE
#include "core/apps/AppManager.hpp"
#include "core/tasks/gui/GuiTask.hpp"
#include "core/ui/LvglBridgeHelpers.hpp"
#endif

static constexpr std::string_view TAG = "SystemManager";

namespace System {

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

#if defined(CONFIG_FLXOS_SD_CARD_ENABLED)
	// Mount SD card (non-critical — failure does not trigger safe mode)
	Services::SdCardService::getInstance().mount();
#endif

	TaskManager::getInstance().initWatchdog();
	ResourceMonitorTask::getInstance().start();
	return ESP_OK;
}

esp_err_t SystemManager::initServices() {
	Log::info(TAG, "Initializing services...");

	// 1. Initialize SettingsManager (Persistence)
	SettingsManager::getInstance().init();

	// 2. Initialize specialized managers
	DisplayManager::getInstance().init();
	ThemeManager::getInstance().init();
	ConnectivityManager::getInstance().init();
	PowerManager::getInstance().init();
	TimeManager::getInstance().init();

	// Start hotspot usage timer (Connectivity handled this inside init, but let's be explicit if needed)
	ConnectivityManager::getInstance().startHotspotUsageTimer();

	Log::info(TAG, "Services initialized");
	return ESP_OK;
}

#if !CONFIG_FLXOS_HEADLESS_MODE
esp_err_t SystemManager::initGuiState() {
	GuiTask::lock();

	INIT_BRIDGE(m_uptime_bridge, m_uptime_subject);

	DisplayManager::getInstance().initGuiBridges();
	ThemeManager::getInstance().initGuiBridges();
	ConnectivityManager::getInstance().initGuiBridges();
	PowerManager::getInstance().initGuiBridges();

	Apps::AppManager::getInstance().init();

	GuiTask::unlock();
	return ESP_OK;
}
#endif

void SystemManager::mount_storage_helper(const char* p, const char* l, wl_handle_t* h, bool f) {
	Log::info(TAG, "Mounting %s...", p);
	esp_vfs_fat_mount_config_t const cfg = {
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

} // namespace System
