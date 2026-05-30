#include <cstddef>
#include <cstdint>
#include <display/lv_display.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <flx/apps/AppManager.hpp>
#include <flx/core/EventBus.hpp>
#include <flx/core/GuiLock.hpp>
#include <flx/core/Logger.hpp>
#include <flx/hal/DeviceRegistry.hpp>
#include <flx/kernel/TaskManager.hpp>
#include <flx/services/ServiceRegistry.hpp>
#include <flx/system/SystemManager.hpp>
#include <flx/system/managers/DisplayManager.hpp>
#include <flx/ui/GuiTask.hpp>
#include <flx/ui/desktop/Desktop.hpp>
#include <flx/ui/theming/UiThemeManager.hpp>
#include <flx/ui/theming/theme_engine/ThemeEngine.hpp>
#include <freertos/idf_additions.h>
#include <freertos/projdefs.h>
#include <lgfx/v1/lgfx_fonts.hpp>
#include <libs/fsdrv/lv_fsdrv.h>
#include <lv_init.h>
#include <misc/lv_timer.h>
#include <misc/lv_types.h>
#include <portmacro.h>
#include <sdkconfig.h>
#include <string>
#include <string_view>
#include <tick/lv_tick.h>

#if !CONFIG_FLXOS_HEADLESS_MODE
#include <Config.hpp>
#include <flx/hal/BusManager.hpp>
#include <flx/hal/display/LgfxDisplayDevice.hpp>
#include <flx/hal/input/IInputDevice.hpp>
#else
#include <flx/hal/display/HeadlessDisplayDevice.hpp>
#endif

static constexpr std::string_view TAG = "GuiTask";

namespace flx::ui {

#if defined(CONFIG_FLXOS_HEADLESS_MODE) && CONFIG_FLXOS_HEADLESS_MODE
static constexpr int kHeadlessMode = 1;
#else
static constexpr int kHeadlessMode = 0;
#endif

bool GuiTask::m_paused = false;
bool GuiTask::m_resume_on_touch = false;
GuiTask::PerfStats GuiTask::m_perfStats {};

GuiTask::GuiTask() : flx::kernel::Task("gui_task", 16 * 1024, 5, 1) {
}

void GuiTask::display_init() {
	lv_init();
	lv_fs_stdio_init();
	Log::info(TAG, "LVGL FS Driver Initialized. Letter: '%c', Path: '%s'", LV_FS_STDIO_LETTER, LV_FS_STDIO_PATH);

	auto& registry = flx::hal::DeviceRegistry::getInstance();

	auto displayDevice = registry.findFirst<flx::hal::display::IDisplayDevice>(flx::hal::IDevice::Type::Display);

	auto isDisplayRegistered = [](lv_display_t* candidate) {
		if (candidate == nullptr) {
			return false;
		}

		for (lv_display_t* current = lv_display_get_next(nullptr);
			 current != nullptr;
			 current = lv_display_get_next(current)) {
			if (current == candidate) {
				return true;
			}
		}

		return false;
	};

	// Fall back to the legacy GUI bootstrap only when profile HWD init did not
	// already register the root display.
	if (!displayDevice) {
#if !CONFIG_FLXOS_HEADLESS_MODE
		displayDevice = std::make_shared<flx::hal::display::LgfxDisplayDevice>();
#else
		displayDevice = std::make_shared<flx::hal::display::HeadlessDisplayDevice>();
#endif
		if (!displayDevice->start()) {
			Log::error(TAG, "Failed to start display device!");
			vTaskDelete(nullptr);
			return;
		}
		registry.registerDevice(displayDevice);
	} else if (displayDevice->getState() != flx::hal::IDevice::State::Ready) {
		if (!displayDevice->start()) {
			Log::error(TAG, "Failed to start pre-registered display device!");
			vTaskDelete(nullptr);
			return;
		}
	}

	lv_display_t* lvDisplay = displayDevice->getLvglDisplay();
	if (!isDisplayRegistered(lvDisplay)) {
		if (lvDisplay != nullptr) {
			Log::warn(TAG, "Display handle is stale after lv_init; restarting display device");
		}

		if (displayDevice->getState() == flx::hal::IDevice::State::Ready) {
			displayDevice->stop();
		}

		if (!displayDevice->start()) {
			Log::error(TAG, "Failed to restart display device after LVGL init");
			vTaskDelete(nullptr);
			return;
		}

		lvDisplay = displayDevice->getLvglDisplay();
	}

	if (lvDisplay == nullptr && kHeadlessMode == 0) {
		Log::error(TAG,
			"Display started but LVGL display handle is null (device='%s', CONFIG_FLXOS_HEADLESS_MODE=%d)",
			std::string(displayDevice->getName()).c_str(),
			kHeadlessMode);
		vTaskDelete(nullptr);
		return;
	}

	if (lv_display_get_default() == nullptr) {
		lv_display_set_default(lvDisplay);
		Log::warn(TAG, "LVGL default display was unset; repaired from HAL device");
	}

	lv_group_t* g = lv_group_create();
	lv_group_set_default(g);

	// Start all registered keyboard input devices
	auto keyboards = registry.findAll<flx::hal::input::IInputDevice>(flx::hal::IDevice::Type::Keyboard);
	for (auto& kbd: keyboards) {
		if (kbd->getState() != flx::hal::IDevice::State::Ready) {
			Log::info(TAG, "Starting keyboard input device: %s", std::string(kbd->getName()).c_str());
			if (kbd->start()) {
				// Link the keypad to the default group so it can control focus
				lv_indev_t* lvkbd = kbd->getLvglIndev();
				if (lvkbd) {
					lv_indev_set_group(lvkbd, g);
				}
			} else {
				Log::error(TAG, "Failed to start keyboard input device: %s", std::string(kbd->getName()).c_str());
			}
		}
	}

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

	uint64_t lastLoopStartUs = static_cast<uint64_t>(esp_timer_get_time());
	while (true) {
		uint64_t const loopStartUs = static_cast<uint64_t>(esp_timer_get_time());
		uint32_t const frameDeltaMs = static_cast<uint32_t>((loopStartUs - lastLoopStartUs) / 1000ULL);
		lastLoopStartUs = loopStartUs;
		heartbeat();
		if (!m_paused) {
			lock();
			uint32_t delay = 10;
			if (!m_paused) {
				uint64_t const handlerStartUs = static_cast<uint64_t>(esp_timer_get_time());
#if !CONFIG_FLXOS_HEADLESS_MODE
				uint64_t const busWaitStartUs = static_cast<uint64_t>(esp_timer_get_time());
				flx::hal::BusManager::ScopedBusLock busLock(flx::config::display.spi.host);
				uint64_t const busWaitUs = static_cast<uint64_t>(esp_timer_get_time()) - busWaitStartUs;
				if (busWaitUs > m_perfStats.maxBusWaitUs) {
					m_perfStats.maxBusWaitUs = busWaitUs;
				}
				if (busLock.isAcquired()) {
					delay = lv_timer_handler();
				}
#else
				delay = lv_timer_handler();
#endif
				UI::Desktop::getInstance().onFrame(frameDeltaMs);
				uint64_t const handlerUs = static_cast<uint64_t>(esp_timer_get_time()) - handlerStartUs;
				if (handlerUs > m_perfStats.maxHandlerUs) {
					m_perfStats.maxHandlerUs = handlerUs;
				}
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

		uint64_t const loopUs = static_cast<uint64_t>(esp_timer_get_time()) - loopStartUs;
		lock();
		++m_perfStats.loopCount;
		if (loopUs > m_perfStats.maxLoopUs) {
			m_perfStats.maxLoopUs = loopUs;
		}
		if (loopUs > 40000ULL) {
			++m_perfStats.over40msCount;
		}
		unlock();
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

GuiTask::PerfStats GuiTask::getPerfStats() {
	lock();
	PerfStats stats = m_perfStats;
	unlock();
	return stats;
}

void GuiTask::resetPerfStats() {
	lock();
	m_perfStats = PerfStats {};
	unlock();
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
