#include "core/common/Logger.hpp"
#include "core/system/Notification/NotificationManager.hpp"
#include "core/system/System/SystemManager.hpp"

#include "core/tasks/gui/GuiTask.hpp"
#include "freertos/task.h"
#include <string_view>

static constexpr std::string_view TAG = "Main";

extern "C" void app_main(void) {
	Log::info(TAG, "Starting FlxOS...");
	System::SystemManager::getInstance().initHardware();

	System::NotificationManager::getInstance().init();
	Log::info(TAG, "Sending welcome notification");
	System::NotificationManager::getInstance().addNotification("Welcome", "FlxOS initialized successfully!", "System", LV_SYMBOL_OK, 1);

	// Test notifications
	for (int i = 0; i < 10; ++i) {
		std::string title = "Notification " + std::to_string(i + 1);
		std::string msg = "This is test notification #" + std::to_string(i + 1) + " for testing scrolling.";
		System::NotificationManager::getInstance().addNotification(title, msg, "TestApp", LV_SYMBOL_TINT, 1);
	}

	Log::info(TAG, "Starting GuiTask...");
	GuiTask* guiTask = new GuiTask();

	guiTask->start();
}
