#include "core/common/Logger.hpp"
#include "core/system/NotificationManager/NotificationManager.hpp"
#include "core/system/SystemManager/SystemManager.hpp"

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

	Log::info(TAG, "Starting GuiTask...");
	GuiTask* guiTask = new GuiTask();

	guiTask->start();
}
