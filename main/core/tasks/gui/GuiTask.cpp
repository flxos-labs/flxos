#include "GuiTask.hpp"
#include <flx/core/Logger.hpp>
#include "core/lv_group.h"
#include "core/system/system_core/SystemManager.hpp"
#include <flx/kernel/TaskManager.hpp>
#include "core/ui/desktop/Desktop.hpp"
#include "core/ui/theming/theme_engine/ThemeEngine.hpp"
#include "display/lv_display.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "lgfx/v1/lgfx_fonts.hpp"
#include "libs/fsdrv/lv_fsdrv.h"
#include "lv_init.h"
#include "misc/lv_timer.h"
#include "misc/lv_types.h"
#include "portmacro.h"
#include "sdkconfig.h"
#include "tick/lv_tick.h"
#include <cstddef>
#include <cstdint>
#include <string_view>

static constexpr std::string_view TAG = "GuiTask";
#include "hal/display/lv_lgfx_user.hpp"
#include "src/drivers/display/lovyan_gfx/lv_lovyan_gfx.h"

SemaphoreHandle_t GuiTask::xGuiSemaphore = nullptr;
lv_display_t* GuiTask::m_disp = nullptr;
bool GuiTask::m_paused = false;
bool GuiTask::m_resume_on_touch = false;

GuiTask::GuiTask() : flx::kernel::Task("gui_task", 32 * 1024, 5, 1) {
	if (!xGuiSemaphore) {
		xGuiSemaphore = xSemaphoreCreateRecursiveMutex();
	}
}

void GuiTask::lock() {
	if (xGuiSemaphore) {
		xSemaphoreTakeRecursive(xGuiSemaphore, portMAX_DELAY);
	}
}
void GuiTask::unlock() {
	if (xGuiSemaphore) {
		xSemaphoreGiveRecursive(xGuiSemaphore);
	}
}

void GuiTask::display_init() {
	lv_init();
	lv_fs_stdio_init();
	const uint32_t SZ =
		CONFIG_FLXOS_DISPLAY_WIDTH * CONFIG_FLXOS_DISPLAY_HEIGHT / 10 * 2;
	void* buf = heap_caps_malloc(SZ, MALLOC_CAP_DMA);
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
		CONFIG_FLXOS_DISPLAY_WIDTH, CONFIG_FLXOS_DISPLAY_HEIGHT, buf, SZ, touch_en
	);
	m_disp = disp;
	if (!disp) {
		Log::error(TAG, "Failed to create display driver!");
		vTaskDelete(nullptr);
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

void GuiTask::run(void* /*data*/) {
	lock();
	Log::info(TAG, "Initializing GUI components...");
	display_init();
	ThemeEngine::init();
	System::SystemManager::getInstance().initGuiState();
	Desktop::getInstance().init();
	unlock();

	Log::info(TAG, "GUI task loop started");
	setWatchdogTimeout(5000);

	while (true) {
		heartbeat();
		if (!m_paused) {
			lock();
			uint32_t delay = 10;
			if (!m_paused) {
				delay = lv_timer_handler();
			}
			unlock();
			vTaskDelay(pdMS_TO_TICKS(delay));
		} else {
			if (m_resume_on_touch) {
				LGFX* tft = getDisplayDriver();
				int32_t x = 0, y = 0;
				if (tft && tft->getTouch(&x, &y)) {
					Log::info(TAG, "Touch detected, resuming LVGL...");
					setResumeOnTouch(false);
					setPaused(false);
				}
			}
			vTaskDelay(pdMS_TO_TICKS(50));
		}
	}
}

LGFX* GuiTask::getDisplayDriver() {
	if (!m_disp) {
		return nullptr;
	}
	// Note: lv_lovyan_gfx.cpp defines lv_lovyan_gfx_t as struct { LGFX* tft; }
	// lv_display_get_driver_data returns void* to this struct.
	auto* driver_data = static_cast<lv_lovyan_gfx_driver_data_t*>(lv_display_get_driver_data(m_disp));
	if (driver_data == nullptr) {
		return nullptr;
	}
	return driver_data->tft;
}

void GuiTask::setPaused(bool paused) {
	lock();
	m_paused = paused;
	if (!paused) {
		lv_obj_invalidate(lv_screen_active());
	}
	unlock();
}

bool GuiTask::isPaused() {
	return m_paused;
}

void GuiTask::setResumeOnTouch(bool enable) {
	m_resume_on_touch = enable;
}

void GuiTask::runDisplayTest(int color) {
	auto* tft = GuiTask::getDisplayDriver();
	if (tft == nullptr) {
		Log::error(TAG, "Failed to get display driver for test!");
		return;
	}

	lock();
	setPaused(true);

	tft->clear(color); // Clear
	tft->setTextDatum(middle_center);
	tft->setTextColor(0xFFFF - color); // Invert text color against bg
	tft->setFont(&Font4);
	tft->drawCentreString("Low Level Driver", tft->width() / 2, tft->height() / 2);
	tft->drawCentreString("Touch to Exit...", tft->width() / 2, (tft->height() / 2) + (tft->fontHeight() * 2));

	setResumeOnTouch(true);
	unlock();
}
