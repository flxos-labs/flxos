#include "Config.hpp"
#include "esp_err.h"
#include "esp_vfs_fat.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "wear_levelling.h"
#include <flx/connectivity/ConnectivityManager.hpp>
#include <flx/core/EventBus.hpp>
#include <flx/core/Logger.hpp>
#include <flx/kernel/ResourceMonitorTask.hpp>
#include <flx/kernel/TaskManager.hpp>
#include <flx/services/ServiceRegistry.hpp>
#include <flx/system/SystemManager.hpp>
#include <flx/system/managers/DisplayManager.hpp>
#include <flx/system/managers/PowerManager.hpp>
#include <flx/system/managers/SettingsManager.hpp>
#include <flx/system/managers/ThemeManager.hpp>
#include <flx/system/managers/TimeManager.hpp>
#if FLXOS_SD_CARD_ENABLED
#include <flx/system/services/SdCardService.hpp>
#endif
#include <flx/system/services/DeviceProfileService.hpp>
#if !CONFIG_FLXOS_HEADLESS_MODE
#include <flx/system/managers/NotificationManager.hpp>
#include <flx/system/services/ScreenshotService.hpp>
#endif
#include <cstring>
#include <memory>
#include <string_view>
#include <sys/stat.h>
#include <unistd.h>

static constexpr std::string_view TAG = "SystemManager";

extern "C" esp_err_t flx_profile_hwd_init(void) __attribute__((weak));

namespace flx::system {

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

	flx::kernel::TaskManager::getInstance().initWatchdog();
	flx::kernel::ResourceMonitorTask::getInstance().start();

	using profile_hwd_init_fn = esp_err_t (*)(void);
	const profile_hwd_init_fn profile_hwd_init = flx_profile_hwd_init;
	if (profile_hwd_init != nullptr) {
		Log::info(TAG, "Running optional profile HWD init...");
		const esp_err_t hwd_err = profile_hwd_init();
		if (hwd_err != ESP_OK) {
			Log::error(TAG, "Profile HWD init failed: %s", esp_err_to_name(hwd_err));
			return hwd_err;
		}
	}

	return ESP_OK;
}

/**
 * Register all system services with the ServiceRegistry.
 * Services declare their own dependencies so the registry resolves boot order.
 */
void SystemManager::registerServices() {
	auto& registry = flx::services::ServiceRegistry::getInstance();

	// Decoupled event publishing
	registry.setEventCallback([](const char* event, const std::string& serviceId) {
		flx::core::Bundle data;
		data.putString("serviceId", serviceId);
		flx::core::EventBus::getInstance().publish(event, data);
	});

	// Core managers (as shared_ptr wrapping the singletons — prevent deletion)
	auto noDelete = [](auto*) {}; // Custom deleter that does nothing
	registry.addService(std::shared_ptr<flx::services::IService>(&SettingsManager::getInstance(), noDelete));
	registry.addService(std::shared_ptr<flx::services::IService>(&DisplayManager::getInstance(), noDelete));
	registry.addService(std::shared_ptr<flx::services::IService>(&ThemeManager::getInstance(), noDelete));
	registry.addService(std::shared_ptr<flx::services::IService>(&flx::connectivity::ConnectivityManager::getInstance(), noDelete));

	registry.addService(std::shared_ptr<flx::services::IService>(&PowerManager::getInstance(), noDelete));
	registry.addService(std::shared_ptr<flx::services::IService>(&TimeManager::getInstance(), noDelete));
	registry.addService(std::shared_ptr<flx::services::IService>(&flx::services::DeviceProfileService::getInstance(), noDelete));

#if FLXOS_SD_CARD_ENABLED
	registry.addService(std::shared_ptr<flx::services::IService>(&flx::services::SdCardService::getInstance(), noDelete));
#endif

#if !CONFIG_FLXOS_HEADLESS_MODE
	registry.addService(std::shared_ptr<flx::services::IService>(&NotificationManager::getInstance(), noDelete));
	registry.addService(std::shared_ptr<flx::services::IService>(&flx::services::ScreenshotService::getInstance(), noDelete));
#endif
}

esp_err_t SystemManager::initServices() {
	Log::info(TAG, "Registering services with ServiceRegistry...");
	registerServices();

	auto& registry = flx::services::ServiceRegistry::getInstance();

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

} // namespace flx::system
