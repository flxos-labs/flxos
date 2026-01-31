#include "core/system/NotificationManager.hpp"
#include "core/system/SystemManager.hpp"

#include "core/tasks/gui/GuiTask.hpp"
#include "freertos/task.h"

extern "C" void app_main(void) {
	ESP_LOGI("Main", "FlxOS starting...");
	System::SystemManager::getInstance().initHardware();
	ESP_LOGI("Main", "Hardware initialization complete");

	System::NotificationManager::getInstance().init();
	System::NotificationManager::getInstance().addNotification("Welcome", "FlxOS initialized successfully!", "System", LV_SYMBOL_OK, 1);

	GuiTask* guiTask = new GuiTask();

	ESP_LOGI("Main", "Starting GUI Task...");
	guiTask->start();
	ESP_LOGI("Main", "FlxOS initialized and running.");
}
