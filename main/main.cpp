#include "core/common/Logger.hpp"
#include "core/system/System/SystemManager.hpp"

#if !CONFIG_FLXOS_HEADLESS_MODE
#include "core/system/Notification/NotificationManager.hpp"
#include "core/tasks/gui/GuiTask.hpp"
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string_view>

static constexpr std::string_view TAG = "Main";

extern "C" void app_main(void) {
	Log::info(TAG, "Starting FlxOS...");
	System::SystemManager::getInstance().initHardware();

#if !CONFIG_FLXOS_HEADLESS_MODE
	System::NotificationManager::getInstance().init();
	Log::info(TAG, "Sending welcome notification");
	System::NotificationManager::getInstance().addNotification("Welcome", "FlxOS initialized successfully!", "System", LV_SYMBOL_OK, 1);

	Log::info(TAG, "Starting GuiTask...");
	GuiTask* guiTask = new GuiTask();
	guiTask->start();
#else
	Log::info(TAG, "Running in headless mode - GUI disabled");
	// In headless mode, just keep the system running
	while (true) {
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
#endif
}
