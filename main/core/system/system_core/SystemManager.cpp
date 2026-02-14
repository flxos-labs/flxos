#include "SystemManager.hpp"
#include <flx/core/Logger.hpp>
#include "core/connectivity/ConnectivityManager.hpp"
#include "core/services/ServiceRegistry.hpp"
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
#include "core/services/device/DeviceProfileService.hpp"
#include "core/services/screenshot/ScreenshotService.hpp"
#if !CONFIG_FLXOS_HEADLESS_MODE
#include "core/system/notification/NotificationManager.hpp"
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

	TaskManager::getInstance().initWatchdog();
	ResourceMonitorTask::getInstance().start();
	return ESP_OK;
}

/**
 * Register all system services with the ServiceRegistry.
 * Services declare their own dependencies so the registry resolves boot order.
 */
void SystemManager::registerServices() {
	auto& registry = Services::ServiceRegistry::getInstance();

	// Core managers (as shared_ptr wrapping the singletons — prevent deletion)
	auto noDelete = [](auto*) {}; // Custom deleter that does nothing
	registry.addService(std::shared_ptr<Services::IService>(&SettingsManager::getInstance(), noDelete));
	registry.addService(std::shared_ptr<Services::IService>(&DisplayManager::getInstance(), noDelete));
	registry.addService(std::shared_ptr<Services::IService>(&ThemeManager::getInstance(), noDelete));
	registry.addService(std::shared_ptr<Services::IService>(&ConnectivityManager::getInstance(), noDelete));
	registry.addService(std::shared_ptr<Services::IService>(&PowerManager::getInstance(), noDelete));
	registry.addService(std::shared_ptr<Services::IService>(&TimeManager::getInstance(), noDelete));
	registry.addService(std::shared_ptr<Services::IService>(&Services::DeviceProfileService::getInstance(), noDelete));

#if defined(CONFIG_FLXOS_SD_CARD_ENABLED)
	registry.addService(std::shared_ptr<Services::IService>(&Services::SdCardService::getInstance(), noDelete));
#endif

#if !CONFIG_FLXOS_HEADLESS_MODE
	registry.addService(std::shared_ptr<Services::IService>(&NotificationManager::getInstance(), noDelete));
	registry.addService(std::shared_ptr<Services::IService>(&Services::ScreenshotService::getInstance(), noDelete));
#endif
}

esp_err_t SystemManager::initServices() {
	Log::info(TAG, "Registering services with ServiceRegistry...");
	registerServices();

	auto& registry = Services::ServiceRegistry::getInstance();

	bool guiMode = true;
#if CONFIG_FLXOS_HEADLESS_MODE
	guiMode = false;
#endif

	bool success = registry.startAll(guiMode);

	if (!success) {
		Log::error(TAG, "Some required services failed — safe mode may be needed");
		m_isSafeMode = true;
	}

	registry.dumpServiceStates();
	Log::info(TAG, "Services initialized via ServiceRegistry");
	return ESP_OK;
}

#if !CONFIG_FLXOS_HEADLESS_MODE
esp_err_t SystemManager::initGuiState() {
	GuiTask::lock();

	INIT_BRIDGE(m_uptime_bridge, m_uptime_subject);

	// Let ServiceRegistry call onGuiInit() on all started services
	Services::ServiceRegistry::getInstance().initGuiServices();

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
