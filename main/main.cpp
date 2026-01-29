#include "core/system/SystemManager.hpp"
#include "core/tasks/gui/GuiTask.hpp"
#include "freertos/task.h"

extern "C" void app_main(void) {
	System::SystemManager::getInstance().initHardware();

	GuiTask* guiTask = new GuiTask();
	guiTask->start();
}
