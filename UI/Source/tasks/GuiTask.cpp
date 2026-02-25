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
#include <flx/apps/AppManager.hpp>
#include <flx/core/EventBus.hpp>
#include <flx/core/GuiLock.hpp>
#include <flx/core/Logger.hpp>
#include <flx/kernel/TaskManager.hpp>
#include <flx/services/ServiceRegistry.hpp>
#include <flx/system/SystemManager.hpp>
#include <flx/system/managers/DisplayManager.hpp>
#include <flx/ui/GuiTask.hpp>
#include <flx/ui/desktop/Desktop.hpp>
#include <flx/ui/theming/UiThemeManager.hpp>
#include <flx/ui/theming/theme_engine/ThemeEngine.hpp>
#include <string_view>

static constexpr std::string_view TAG = "GuiTask";
#include <flx/hal/DeviceRegistry.hpp>
#if !CONFIG_FLXOS_HEADLESS_MODE
#include "Config.hpp"
#include <flx/hal/display/LgfxDisplayDevice.hpp>
#else
#include <flx/hal/display/HeadlessDisplayDevice.hpp>
#endif

namespace flx::ui {

bool GuiTask::m_paused = false;
bool GuiTask::m_resume_on_touch = false;

GuiTask::GuiTask() : flx::kernel::Task("gui_task", 32 * 1024, 5, 1) {
}

void GuiTask::display_init() {
	lv_init();
	lv_fs_stdio_init();
	Log::info(TAG, "LVGL FS Driver Initialized. Letter: '%c', Path: '%s'", LV_FS_STDIO_LETTER, LV_FS_STDIO_PATH);

	auto& registry = flx::hal::DeviceRegistry::getInstance();

	// Instantiate the root display device (Until Phase 12 orchestrator is built)
#if !CONFIG_FLXOS_HEADLESS_MODE
	auto displayDevice = std::make_shared<flx::hal::display::LgfxDisplayDevice>();
#else
	auto displayDevice = std::make_shared<flx::hal::display::HeadlessDisplayDevice>();
#endif

	registry.registerDevice(displayDevice);

	if (!displayDevice->start()) {
		Log::error(TAG, "Failed to start display device!");
		vTaskDelete(nullptr);
		return;
	}

	lv_group_t* g = lv_group_create();
	lv_group_set_default(g);

	lv_tick_set_cb([]() { return (uint32_t)(esp_timer_get_time() / 1000); });
}

void GuiTask::run(void* /*data*/) {
	lock();
	Log::info(TAG, "Initializing GUI components...");
	display_init();
	::ThemeEngine::init();

	// Initialize GUI-dependent services and apps
	flx::services::ServiceRegistry::getInstance().initGuiServices();
	flx::apps::AppManager::getInstance().init();

	flx::ui::theming::UiThemeManager::getInstance().init();
	UI::Desktop::getInstance().init();

	// Initialize brightness from settings
	auto& displayMgr = flx::system::DisplayManager::getInstance();
	auto& brightnessObs = displayMgr.getBrightnessObservable();

	auto displayDev = flx::hal::DeviceRegistry::getInstance().findFirst<flx::hal::display::IDisplayDevice>(flx::hal::IDevice::Type::Display);
	auto lv_disp = displayDev ? displayDev->getLvglDisplay() : nullptr;

	// Apply initial value
	if (displayDev) {
		displayDev->setBacklightDuty(brightnessObs.get());
	}

	// Subscribe to changes
	brightnessObs.subscribe([displayDev](const int32_t& val) {
		GuiTask::perform([displayDev, val]() {
			if (displayDev) {
				displayDev->setBacklightDuty(val);
			}
		});
	});

	// Initialize rotation from settings
	auto& rotationObs = displayMgr.getRotationObservable();

	// Apply initial value
	int32_t currentRotation = rotationObs.get();
	if (lv_disp) {
		lv_display_set_rotation(lv_disp, (lv_display_rotation_t)(currentRotation / 90));
	}

	// Subscribe to changes
	rotationObs.subscribe([lv_disp](const int32_t& val) {
		GuiTask::perform([lv_disp, val]() {
			if (lv_disp) {
				lv_display_set_rotation(lv_disp, (lv_display_rotation_t)(val / 90));
			}
		});
	});

	// Initialize Show FPS from settings
	auto& showFpsObs = displayMgr.getShowFpsObservable();

	// Apply initial value
	int32_t showFps = showFpsObs.get();
	if (lv_disp) {
#if LV_USE_SYSMON
		if (showFps) {
			lv_sysmon_show_performance(lv_disp);
		} else {
			lv_sysmon_hide_performance(lv_disp);
		}
#endif
	}

	// Subscribe to changes
	showFpsObs.subscribe([lv_disp](const int32_t& val) {
		GuiTask::perform([lv_disp, val]() {
			if (lv_disp) {
#if LV_USE_SYSMON
				if (val) {
					lv_sysmon_show_performance(lv_disp);
				} else {
					lv_sysmon_hide_performance(lv_disp);
				}
#endif
			}
		});
	});

	// Subscribe to GUI control events
	flx::core::EventBus::getInstance().subscribe("ui.gui.set_paused", [](const std::string& /*event*/, const flx::core::Bundle& data) {
		setPaused(data.getBool("paused"));
	});

	flx::core::EventBus::getInstance().subscribe("ui.gui.run_display_test", [](const std::string& /*event*/, const flx::core::Bundle& data) {
		runDisplayTest(data.getInt32("color"));
	});

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
#if !CONFIG_FLXOS_HEADLESS_MODE
				auto displayDev = flx::hal::DeviceRegistry::getInstance().findFirst<flx::hal::display::LgfxDisplayDevice>(flx::hal::IDevice::Type::Display);
				if (displayDev) {
					auto* tft = displayDev->getRawDriver();
					int32_t x = 0, y = 0;
					if (tft && tft->getTouch(&x, &y)) {
						Log::info(TAG, "Touch detected, resuming LVGL...");
						setResumeOnTouch(false);
						setPaused(false);
					}
				}
#endif
			}
			vTaskDelay(pdMS_TO_TICKS(50));
		}
	}
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
	auto displayDev = flx::hal::DeviceRegistry::getInstance().findFirst<flx::hal::display::IDisplayDevice>(flx::hal::IDevice::Type::Display);

	if (!displayDev) {
		Log::error(TAG, "Failed to get display driver for test!");
		return;
	}

	lock();
	setPaused(true);

	displayDev->runColorTest(color);

	setResumeOnTouch(true);
	unlock();
}

} // namespace flx::ui
