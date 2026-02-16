#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <flx/core/GuiLock.hpp>
#include <flx/kernel/TaskManager.hpp>
#include <functional>

typedef struct _lv_display_t lv_display_t;

class LGFX;

namespace flx::ui {

class GuiTask : public flx::kernel::Task {
public:

	GuiTask();
	~GuiTask() override = default;

	static void lock() { flx::core::GuiLock::lock(); }
	static void unlock() { flx::core::GuiLock::unlock(); }

	static void perform(std::function<void()> func) {
		lock();
		func();
		unlock();
	}

	static LGFX* getDisplayDriver();
	static void setPaused(bool paused);
	static void setResumeOnTouch(bool enable);
	static void runDisplayTest(int color);
	static bool isPaused();

protected:

	void run(void* data) override;

private:

	static void display_init();

	static lv_display_t* m_disp;
	static bool m_paused;
	static bool m_resume_on_touch;
};

} // namespace flx::ui
