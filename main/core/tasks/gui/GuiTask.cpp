#include "GuiTask.hpp"
#include "LovyanGFX.hpp"
#include "core/apps/AppManager.hpp"
#include "core/system/SystemManager.hpp"
#include "core/ui/DE/DE.hpp"
#include "core/ui/theming/ThemeEngine.hpp"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "lvgl.h"
#include "src/drivers/display/lovyan_gfx/lv_lovyan_gfx.h"

SemaphoreHandle_t GuiTask::xGuiSemaphore = nullptr;

GuiTask::GuiTask() : System::Task("gui_task", 20 * 1024, 5, 1) {
	if (!xGuiSemaphore)
		xGuiSemaphore = xSemaphoreCreateRecursiveMutex();
}

void GuiTask::lock() {
	if (xGuiSemaphore)
		xSemaphoreTakeRecursive(xGuiSemaphore, portMAX_DELAY);
}
void GuiTask::unlock() {
	if (xGuiSemaphore)
		xSemaphoreGiveRecursive(xGuiSemaphore);
}

void GuiTask::display_init() {
	lv_init();
	lv_fs_stdio_init();
	const uint32_t sz =
		CONFIG_OS_DISPLAY_WIDTH * CONFIG_OS_DISPLAY_HEIGHT / 10 * 2;
	void* buf = heap_caps_malloc(sz, MALLOC_CAP_DMA);
	if (!buf)
		ESP_LOGE("GuiTask", "Failed to allocate display buffer!");

	bool touch_en = false;
#if CONFIG_OS_HARDWARE_AUTODETECT
	touch_en = true;
#elif CONFIG_OS_TOUCH_ENABLED
	touch_en = true;
#endif

	lv_display_t* disp = lv_lovyan_gfx_create(
		CONFIG_OS_DISPLAY_WIDTH, CONFIG_OS_DISPLAY_HEIGHT, buf, sz, touch_en
	);
	if (!disp) {
		ESP_LOGE("GuiTask", "Display initialization failed! Halting.");
		vTaskDelete(NULL);
		return;
	}
	lv_display_set_rotation(
		disp, (lv_display_rotation_t)(CONFIG_OS_DISPLAY_ROTATION / 90)
	);

	lv_group_t* g = lv_group_create();
	lv_group_set_default(g);

	lv_tick_set_cb([]() { return (uint32_t)(esp_timer_get_time() / 1000); });
}

void GuiTask::run(void*) {
	lock();
	display_init();
	ThemeEngine::init();
	System::SystemManager::getInstance().initGuiState();
	DE::getInstance().init();
	unlock();

	setWatchdogTimeout(5000);

	while (true) {
		heartbeat();
		lock();
		uint32_t delay = lv_timer_handler();
		unlock();
		vTaskDelay(pdMS_TO_TICKS(delay));
	}
}