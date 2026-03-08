#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
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

	static bool isHeldByCurrentTask() {
		auto& instance = getInstance();
		return instance.m_semaphore && xSemaphoreGetMutexHolder(instance.m_semaphore) == xTaskGetCurrentTaskHandle();
	}

	static UBaseType_t releaseAllForCurrentTask() {
		auto& instance = getInstance();
		if (!instance.m_semaphore) {
			return 0;
		}

		UBaseType_t releaseCount = 0;
		TaskHandle_t current = xTaskGetCurrentTaskHandle();
		while (xSemaphoreGetMutexHolder(instance.m_semaphore) == current) {
			xSemaphoreGiveRecursive(instance.m_semaphore);
			++releaseCount;
		}
		return releaseCount;
	}

	static void reacquireForCurrentTask(UBaseType_t count) {
		auto& instance = getInstance();
		if (!instance.m_semaphore) {
			return;
		}

		while (count-- > 0) {
			xSemaphoreTakeRecursive(instance.m_semaphore, portMAX_DELAY);
		}
	}

private:

	GuiLock() {
		m_semaphore = xSemaphoreCreateRecursiveMutex();
		configASSERT(m_semaphore != nullptr);
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
