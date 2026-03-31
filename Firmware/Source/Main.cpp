#include "sdkconfig.h"
#include <Config.hpp>

// Profile must be selected at build time via profile.yaml
static_assert(flx::config::profile.id[0] != '\0', "No device profile selected.");

#include <flx/core/Logger.hpp>
#include <flx/system/SystemManager.hpp>

#if CONFIG_LV_USE_LOVYAN_GFX
#include "font/lv_symbol_def.h"
#include <flx/system/managers/NotificationManager.hpp>
#include <flx/ui/GuiTask.hpp>

namespace System::Apps {
void registerBuiltInApps();
}
#endif

#include <flx/services/ServiceRegistry.hpp>
#include <flx/system/services/CliService.hpp>
#include <memory>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string_view>

static constexpr std::string_view TAG = "Main";

extern "C" void app_main(void) {
	Log::info(TAG, "Starting FlxOS...");
	flx::system::SystemManager::getInstance().initHardware();
	flx::system::SystemManager::getInstance().initServices();

	if constexpr (flx::config::cli.enabled) {
		// CLI is autoStart = false, so start it explicitly via ServiceRegistry
		auto noDelete = [](auto*) {};
		auto& registry = flx::services::ServiceRegistry::getInstance();
		registry.addService(std::shared_ptr<flx::services::IService>(&flx::system::CliService::getInstance(), noDelete));
		flx::system::CliService::getInstance().start();
	}

#if CONFIG_LV_USE_LOVYAN_GFX
	// Register built-in apps with the AppRegistry
	Log::info(TAG, "Registering apps with AppRegistry...");
	System::Apps::registerBuiltInApps();

	// Initial welcome notification
	Log::info(TAG, "Sending welcome notification");
	flx::system::NotificationManager::getInstance().addNotification(
		"Welcome",
		"FlxOS initialized successfully!",
		"System",
		LV_SYMBOL_OK,
		1);

	// Start GUI task
	Log::info(TAG, "Starting GuiTask...");
	auto* guiTask = new flx::ui::GuiTask();
	guiTask->start();
#else
	// Headless mode behavior
	Log::info(TAG, "Running in headless mode - GUI disabled");
	Log::info(TAG, "Services initialized: WiFi, Hotspot, Bluetooth available");

	// In headless mode with CLI, the REPL runs its own loop.
	// If CLI is disabled, keep the task alive explicitly.
	if constexpr (!flx::config::cli.enabled) {
		while (true) {
			vTaskDelay(pdMS_TO_TICKS(1000));
		}
	}
#endif
}
