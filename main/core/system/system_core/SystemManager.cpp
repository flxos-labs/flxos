#include "SystemManager.hpp"
#include "core/system/display/DisplayManager.hpp"
#include "core/system/power/PowerManager.hpp"
#include "core/system/settings/SettingsManager.hpp"
#include "core/system/theme/ThemeManager.hpp"
#include "core/system/time/TimeManager.hpp"
#include "esp_err.h"
#include "esp_vfs_fat.h"
#include "flx/connectivity/ConnectivityManager.hpp"
#include "nvs.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "wear_levelling.h"
#include <flx/apps/EventBus.hpp>
#include <flx/core/Logger.hpp>
#include <flx/kernel/ResourceMonitorTask.hpp>
#include <flx/kernel/TaskManager.hpp>
#include <flx/services/ServiceRegistry.hpp>
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
#include "core/tasks/gui/GuiTask.hpp"
#include "core/ui/LvglBridgeHelpers.hpp"
#include <flx/apps/AppManager.hpp>
#include <flx/apps/AppRegistry.hpp>

// App includes for registration
#include "core/apps/calendar/CalendarApp.hpp"
#include "core/apps/files/FilesApp.hpp"
#include "core/apps/image_viewer/ImageViewerApp.hpp"
#include "core/apps/settings/SettingsApp.hpp"
#include "core/apps/system_info/SystemInfoApp.hpp"
#include "core/apps/text_editor/TextEditorApp.hpp"
#include "core/apps/tools/ToolsApp.hpp"
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

	flx::kernel::TaskManager::getInstance().initWatchdog();
	flx::kernel::ResourceMonitorTask::getInstance().start();
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
		flx::apps::Bundle data;
		data.putString("serviceId", serviceId);
		flx::apps::EventBus::getInstance().publish(event, data);
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

#if defined(CONFIG_FLXOS_SD_CARD_ENABLED)
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

#if !CONFIG_FLXOS_HEADLESS_MODE
esp_err_t SystemManager::initGuiState() {
	GuiTask::lock();

	INIT_BRIDGE(m_uptime_bridge, m_uptime_subject);

	// Let ServiceRegistry call onGuiInit() on all started services
	flx::services::ServiceRegistry::getInstance().initGuiServices();

	// Register built-in apps
	auto& registry = flx::apps::AppRegistry::getInstance();
	registry.addApp(System::Apps::SettingsApp::manifest);
	registry.addApp(System::Apps::FilesApp::manifest);
	registry.addApp(System::Apps::SystemInfoApp::manifest);
	registry.addApp(System::Apps::CalendarApp::manifest);
	registry.addApp(System::Apps::TextEditorApp::manifest);
	registry.addApp(System::Apps::ToolsApp::manifest);
	registry.addApp(System::Apps::ImageViewerApp::manifest);

	flx::apps::AppManager::getInstance().init();

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
