#pragma once

#include "../TaskManager.hpp"
#include "freertos/semphr.h"

class GuiTask : public System::Task {
public:

	GuiTask();
	virtual ~GuiTask() = default;

	static void lock();
	static void unlock();

protected:

	void run(void* data) override;

private:

	void display_init();
	static SemaphoreHandle_t xGuiSemaphore;
};
