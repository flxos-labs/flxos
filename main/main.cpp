#include "core/system/NotificationManager.hpp"
#include "core/system/SystemManager.hpp"

#include "core/tasks/gui/GuiTask.hpp"
#include "freertos/task.h"

extern "C" void app_main(void) {
	System::SystemManager::getInstance().initHardware();

	System::NotificationManager::getInstance().init();
	System::NotificationManager::getInstance().addNotification("Welcome", "FlxOS initialized successfully!", "System", LV_SYMBOL_OK, 1);

	GuiTask* guiTask = new GuiTask();

	guiTask->start();
}
