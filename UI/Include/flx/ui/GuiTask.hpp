#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <cstdint>
#include <flx/core/GuiLock.hpp>
#include <flx/kernel/TaskManager.hpp>
#include <functional>

typedef struct _lv_display_t lv_display_t;

class LGFX;

namespace flx::ui {

class GuiTask : public flx::kernel::Task {
public:

	struct PerfStats {
		uint64_t loopCount {0};
		uint64_t over40msCount {0};
		uint64_t maxLoopUs {0};
		uint64_t maxHandlerUs {0};
		uint64_t maxBusWaitUs {0};
	};

	GuiTask();
	~GuiTask() override = default;

	static void lock() { flx::core::GuiLock::lock(); }
	static void unlock() { flx::core::GuiLock::unlock(); }

	static void perform(std::function<void()> func) {
		lock();
		func();
		unlock();
	}

	static void setPaused(bool paused);
	static void setResumeOnTouch(bool enable);
	static void runDisplayTest(int color);
	static bool isPaused();
	static PerfStats getPerfStats();
	static void resetPerfStats();

protected:

	void run(void* data) override;

private:

	static void display_init();

	static bool m_paused;
	static bool m_resume_on_touch;
	static PerfStats m_perfStats;
};

} // namespace flx::ui
