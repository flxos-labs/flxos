#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <flx/core/Singleton.hpp>

namespace flx::core {

class GuiLock : public flx::Singleton<GuiLock> {
	friend class flx::Singleton<GuiLock>;

public:

	static void lock() {
		auto& instance = getInstance();
		if (instance.m_semaphore) {
			xSemaphoreTakeRecursive(instance.m_semaphore, portMAX_DELAY);
		}
	}

	static void unlock() {
		auto& instance = getInstance();
		if (instance.m_semaphore) {
			xSemaphoreGiveRecursive(instance.m_semaphore);
		}
	}

private:

	GuiLock() {
		m_semaphore = xSemaphoreCreateRecursiveMutex();
	}
	~GuiLock() {
		if (m_semaphore) {
			vSemaphoreDelete(m_semaphore);
			m_semaphore = nullptr;
		}
	}

	SemaphoreHandle_t m_semaphore = nullptr;
};

} // namespace flx::core
