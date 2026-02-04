#pragma once

#include "../TaskManager.hpp"
#include "freertos/semphr.h"

class GuiTask : public System::Task {
public:

	GuiTask();
	~GuiTask() override = default;

	static void lock();
	static void unlock();

protected:

	void run(void* data) override;

private:

	static void display_init();
	static SemaphoreHandle_t xGuiSemaphore;
};
