#include "GuiTask.hpp"
#include "core/common/Logger.hpp"
#include "core/system/SystemManager.hpp"
#include "core/ui/DE/DE.hpp"
#include "core/ui/theming/ThemeEngine.hpp"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include <string_view>

static constexpr std::string_view TAG = "GuiTask";
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
		CONFIG_FLXOS_DISPLAY_WIDTH * CONFIG_FLXOS_DISPLAY_HEIGHT / 10 * 2;
	void* buf = heap_caps_malloc(sz, MALLOC_CAP_DMA);
	if (!buf) {
	} else {
	}

	bool touch_en = false;
#if CONFIG_FLXOS_HARDWARE_AUTODETECT
	touch_en = true;
#elif CONFIG_FLXOS_TOUCH_ENABLED
	touch_en = true;
#endif

	lv_display_t* disp = lv_lovyan_gfx_create(
		CONFIG_FLXOS_DISPLAY_WIDTH, CONFIG_FLXOS_DISPLAY_HEIGHT, buf, sz, touch_en
	);
	if (!disp) {
		Log::error(TAG, "Failed to create display driver!");
		vTaskDelete(NULL);
		return;
	}
	Log::info("GuiTask", "Display initialized: %dx%d", CONFIG_FLXOS_DISPLAY_WIDTH, CONFIG_FLXOS_DISPLAY_HEIGHT);
	lv_display_set_rotation(
		disp, (lv_display_rotation_t)(CONFIG_FLXOS_DISPLAY_ROTATION / 90)
	);

	lv_group_t* g = lv_group_create();
	lv_group_set_default(g);

	lv_tick_set_cb([]() { return (uint32_t)(esp_timer_get_time() / 1000); });
}

void GuiTask::run(void*) {
	lock();
	Log::info(TAG, "Initializing GUI components...");
	display_init();
	ThemeEngine::init();
	System::SystemManager::getInstance().initGuiState();
	DE::getInstance().init();
	unlock();

	Log::info(TAG, "GUI task loop started");
	setWatchdogTimeout(5000);

	while (true) {
		heartbeat();
		lock();
		uint32_t delay = lv_timer_handler();
		unlock();
		vTaskDelay(pdMS_TO_TICKS(delay));
	}
}
