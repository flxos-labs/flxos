#include "sdkconfig.h"
#include <flx/core/Compat.hpp> // Namespace compatibility during migration
#include <flx/core/Logger.hpp>
#include <flx/system/SystemManager.hpp>

#if !CONFIG_FLXOS_HEADLESS_MODE
#include "font/lv_symbol_def.h"
#include <flx/system/managers/NotificationManager.hpp>
#include <flx/ui/GuiTask.hpp>
#endif

#if !CONFIG_FLXOS_HEADLESS_MODE
#include "core/apps/calendar/CalendarApp.hpp"
#include "core/apps/files/FilesApp.hpp"
#include "core/apps/image_viewer/ImageViewerApp.hpp"
#include "core/apps/settings/SettingsApp.hpp"
#include "core/apps/system_info/SystemInfoApp.hpp"
#include "core/apps/text_editor/TextEditorApp.hpp"
#include "core/apps/tools/ToolsApp.hpp"
#include <flx/ui/app/AppRegistry.hpp>
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
	// CLI is autoStart=false, so start it explicitly
	auto noDelete = [](auto*) {};
	auto& registry = flx::services::ServiceRegistry::getInstance();
	registry.addService(std::shared_ptr<flx::services::IService>(&flx::system::CliService::getInstance(), noDelete));
	flx::system::CliService::getInstance().start();
#endif

#if !CONFIG_FLXOS_HEADLESS_MODE
	Log::info(TAG, "Registering apps with AppRegistry...");
	auto& appRegistry = flx::app::AppRegistry::getInstance();
	appRegistry.addApp(System::Apps::SettingsApp::manifest);
	appRegistry.addApp(System::Apps::CalendarApp::manifest);
	appRegistry.addApp(System::Apps::FilesApp::manifest);
	appRegistry.addApp(System::Apps::ImageViewerApp::manifest);
	appRegistry.addApp(System::Apps::SystemInfoApp::manifest);
	appRegistry.addApp(System::Apps::TextEditorApp::manifest);
	appRegistry.addApp(System::Apps::ToolsApp::manifest);

	Log::info(TAG, "Sending welcome notification");
	flx::system::NotificationManager::getInstance().addNotification("Welcome", "FlxOS initialized successfully!", "System", LV_SYMBOL_OK, 1);

	Log::info(TAG, "Starting GuiTask...");
	auto* guiTask = new flx::ui::GuiTask();
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
