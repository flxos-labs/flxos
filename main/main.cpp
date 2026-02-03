#include "core/common/Logger.hpp"
#include "core/system/system_core/SystemManager.hpp"

#if !CONFIG_FLXOS_HEADLESS_MODE
#include "core/system/notification/NotificationManager.hpp"
#include "core/tasks/gui/GuiTask.hpp"
#endif

#if CONFIG_FLXOS_CLI_ENABLED
#include "core/services/cli/CliService.hpp"
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string_view>

static constexpr std::string_view TAG = "Main";

extern "C" void app_main(void) {
	Log::info(TAG, "Starting FlxOS...");
	System::SystemManager::getInstance().initHardware();
	System::SystemManager::getInstance().initServices();

#if CONFIG_FLXOS_CLI_ENABLED
	// Initialize CLI (works in both headless and GUI modes)
	System::CliService::getInstance().init();
#endif

#if !CONFIG_FLXOS_HEADLESS_MODE
	System::NotificationManager::getInstance().init();
	Log::info(TAG, "Sending welcome notification");
	System::NotificationManager::getInstance().addNotification("Welcome", "FlxOS initialized successfully!", "System", LV_SYMBOL_OK, 1);

	Log::info(TAG, "Starting GuiTask...");
	GuiTask* guiTask = new GuiTask();
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
