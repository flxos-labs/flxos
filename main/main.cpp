#include <flx/core/Logger.hpp>
#include <flx/core/Compat.hpp>  // Namespace compatibility during migration
#include "core/system/system_core/SystemManager.hpp"
#include "sdkconfig.h"

#if !CONFIG_FLXOS_HEADLESS_MODE
#include "core/system/notification/NotificationManager.hpp"
#include "core/tasks/gui/GuiTask.hpp"
#include "font/lv_symbol_def.h"
#endif

#if CONFIG_FLXOS_CLI_ENABLED
#include <flx/services/ServiceRegistry.hpp>
#include "core/services/cli/CliService.hpp"
#include <memory>
#endif

#include "freertos/task.h"
#include <string_view>

static constexpr std::string_view TAG = "Main";

extern "C" void app_main(void) {
	Log::info(TAG, "Starting FlxOS...");
	System::SystemManager::getInstance().initHardware();
	System::SystemManager::getInstance().initServices();

#if CONFIG_FLXOS_CLI_ENABLED
	// CLI is autoStart=false, so start it explicitly
	auto noDelete = [](auto*) {};
	auto& registry = flx::services::ServiceRegistry::getInstance();
	registry.addService(std::shared_ptr<flx::services::IService>(&System::CliService::getInstance(), noDelete));
	System::CliService::getInstance().start();
#endif

#if !CONFIG_FLXOS_HEADLESS_MODE
	Log::info(TAG, "Sending welcome notification");
	System::NotificationManager::getInstance().addNotification("Welcome", "FlxOS initialized successfully!", "System", LV_SYMBOL_OK, 1);

	Log::info(TAG, "Starting GuiTask...");
	auto* guiTask = new GuiTask();
	guiTask->start();
#else
	Log::info(TAG, "Running in headless mode - GUI disabled");
	Log::info(TAG, "Services initialized: WiFi, Hotspot, Bluetooth available");
	// In headless mode with CLI, the REPL runs its own loop
	// If CLI is disabled, just keep the task alive
#if !CONFIG_FLXOS_CLI_ENABLED
	while (true) {
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
#endif
#endif
}
