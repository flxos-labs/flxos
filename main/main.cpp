#include "core/system/SystemManager.hpp"
#include "core/tasks/gui/GuiTask.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

extern "C" void app_main(void) {
	System::SystemManager::getInstance().initHardware();

	GuiTask* guiTask = new GuiTask();
	guiTask->start();
}