#pragma once

#include <flx/kernel/TaskManager.hpp>
#include "freertos/semphr.h"

typedef struct _lv_display_t lv_display_t;

class GuiTask : public flx::kernel::Task {
public:

	GuiTask();
	~GuiTask() override = default;

	static void lock();
	static void unlock();

	static class LGFX* getDisplayDriver();
	static void setPaused(bool paused);
	static void setResumeOnTouch(bool enable);
	static void runDisplayTest(int color);
	static bool isPaused();

protected:

	void run(void* data) override;

private:

	static void display_init();
	static SemaphoreHandle_t xGuiSemaphore;
	static lv_display_t* m_disp;
	static bool m_paused;
	static bool m_resume_on_touch;
};
