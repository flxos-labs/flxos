#include "sdkconfig.h"

#include <flx/core/Logger.hpp>
#include <flx/system/SystemManager.hpp>

#if !CONFIG_FLXOS_HEADLESS_MODE
#include "font/lv_symbol_def.h"
#include <flx/apps/AppRegistry.hpp>
#include <flx/system/managers/NotificationManager.hpp>
#include <flx/ui/GuiTask.hpp>

#include "calendar/CalendarApp.hpp"
#include "files/FilesApp.hpp"
#include "image_viewer/ImageViewerApp.hpp"
#include "settings/SettingsApp.hpp"
#include "system_info/SystemInfoApp.hpp"
#include "text_editor/TextEditorApp.hpp"
#include "tools/ToolsApp.hpp"
#endif

#if CONFIG_FLXOS_CLI_ENABLED
#include <flx/services/ServiceRegistry.hpp>
#include <flx/system/services/CliService.hpp>
#include <memory>
#endif

#include "freertos/task.h"
#include <string_view>

static constexpr std::string_view TAG = "Main";

extern "C" void app_main(void) {
	Log::info(TAG, "Starting FlxOS...");
	flx::system::SystemManager::getInstance().initHardware();
	flx::system::SystemManager::getInstance().initServices();

#if CONFIG_FLXOS_CLI_ENABLED
	// CLI is autoStart = false, so start it explicitly via ServiceRegistry
	auto noDelete = [](auto*) {};
	auto& registry = flx::services::ServiceRegistry::getInstance();
	registry.addService(std::shared_ptr<flx::services::IService>(&flx::system::CliService::getInstance(), noDelete));
	flx::system::CliService::getInstance().start();
#endif

#if !CONFIG_FLXOS_HEADLESS_MODE
	// Register built-in apps with the AppRegistry
	Log::info(TAG, "Registering apps with AppRegistry...");
	auto& appRegistry = flx::apps::AppRegistry::getInstance();
	appRegistry.addApp(System::Apps::SettingsApp::manifest);
	appRegistry.addApp(System::Apps::CalendarApp::manifest);
	appRegistry.addApp(System::Apps::FilesApp::manifest);
	appRegistry.addApp(System::Apps::ImageViewerApp::manifest);
	appRegistry.addApp(System::Apps::SystemInfoApp::manifest);
	appRegistry.addApp(System::Apps::TextEditorApp::manifest);
	appRegistry.addApp(System::Apps::ToolsApp::manifest);

	// Initial welcome notification
	Log::info(TAG, "Sending welcome notification");
	flx::system::NotificationManager::getInstance().addNotification(
		"Welcome",
		"FlxOS initialized successfully!",
		"System",
		LV_SYMBOL_OK,
		1
	);

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
#if !CONFIG_FLXOS_CLI_ENABLED
	while (true) {
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
#endif
#endif
}
