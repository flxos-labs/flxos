#include "TaskManager.hpp"
#include "FreeRTOSConfig.h"
#include "core/common/Logger.hpp"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "portmacro.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>

static constexpr std::string_view TASK_TAG = "Task";
static constexpr std::string_view TM_TAG = "TaskManager";

namespace System {
static uint64_t getMillis() { return esp_timer_get_time() / 1000; }

Task::Task(const std::string& name, uint32_t stackSize, UBaseType_t priority, BaseType_t coreId)
	: m_name(name), m_stackSize(stackSize), m_priority(priority),
	  m_coreId(coreId) {
	TaskManager::getInstance().registerTask(this);
}

Task::~Task() {
	stop();
	TaskManager::getInstance().unregisterTask(this);
}

bool Task::start(void* data) {
	TaskHandle_t expected = nullptr;
	if (!m_handle.compare_exchange_strong(expected, (TaskHandle_t)1)) {
		return false;
	}

	m_data = data;
	m_stopRequested = false;
	m_lastHeartbeat = getMillis();

	TaskHandle_t handle = nullptr;
	BaseType_t res = (m_coreId == tskNO_AFFINITY)
		? xTaskCreate(taskEntry, m_name.c_str(), m_stackSize, this, m_priority, &handle)
		: xTaskCreatePinnedToCore(taskEntry, m_name.c_str(), m_stackSize, this, m_priority, &handle, m_coreId);

	if (res != pdPASS) {
		Log::error(TASK_TAG, "Failed to create task: %s", m_name.c_str());
		m_handle = nullptr;
		return false;
	}

	expected = (TaskHandle_t)1;
	if (!m_handle.compare_exchange_strong(expected, handle)) {
		// stop() was called during creation
		vTaskDelete(handle);
	} else {
		Log::info(TASK_TAG, "Task started: %s", m_name.c_str());
	}
	return true;
}

void Task::stop() {
	m_stopRequested = true;
	TaskHandle_t handle = m_handle.exchange(nullptr);
	if (handle && handle != (TaskHandle_t)1) {
		Log::info(TASK_TAG, "Task stopped: %s", m_name.c_str());
		vTaskDelete(handle);
	}
}

void Task::join() const {
	while (isRunning()) {
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

void Task::taskEntry(void* param) {
	Task* t = static_cast<Task*>(param);
	if (t) {
		t->run(t->m_data);
		TaskHandle_t handle = t->m_handle.exchange(nullptr);
		if (handle) {
			// If we got the handle, it means stop() wasn't called from another thread
			// OR stop() was called and got nullptr because we exchanged it first.
			// But we are the task itself, so if we call vTaskDelete(NULL) it's fine.
		}
		vTaskDelete(nullptr);
	}
}

TaskManager& TaskManager::getInstance() {
	static TaskManager instance;
	return instance;
}

void TaskManager::registerTask(Task* t) {
	std::lock_guard<std::mutex> lock(m_mutex);
	if (std::find(m_tasks.begin(), m_tasks.end(), t) == m_tasks.end())
		m_tasks.push_back(t);
}

void TaskManager::unregisterTask(Task* t) {
	std::lock_guard<std::mutex> lock(m_mutex);
	m_tasks.erase(std::remove(m_tasks.begin(), m_tasks.end(), t), m_tasks.end());
}

Task* TaskManager::getTask(const std::string& name) {
	std::lock_guard<std::mutex> lock(m_mutex);
	for (auto* t: m_tasks)
		if (t->getName() == name)
			return t;
	return nullptr;
}

void TaskManager::initWatchdog(uint32_t interval) {
	m_checkIntervalMs = interval;

	esp_task_wdt_config_t const twdt_config = {
		.timeout_ms = interval * 3, // Hardware timeout 3x software check
		.idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
		.trigger_panic = true,
	};

	if (esp_task_wdt_reconfigure(&twdt_config) != ESP_OK) {
		esp_task_wdt_init(&twdt_config);
	}

	Log::info(TM_TAG, "Watchdog initialized with %d ms interval", (int)interval);

	if (!m_watchdogTaskHandle) {
		BaseType_t const res = xTaskCreatePinnedToCore(
			watchdogTaskEntry, "tm_watchdog", 3072, this, configMAX_PRIORITIES - 1,
			&m_watchdogTaskHandle, 0
		);
		if (res != pdPASS) {
		}
	}
}

void TaskManager::watchdogTaskEntry(void* p) {
	auto* tm = static_cast<TaskManager*>(p);
	esp_task_wdt_add(nullptr); // Add this task to TWDT
	while (true) {
		tm->checkTasks(getMillis());
		if (!tm->checkHeapIntegrity()) {
			esp_restart();
		}
		esp_task_wdt_reset(); // Hardware kick
		vTaskDelay(pdMS_TO_TICKS(tm->m_checkIntervalMs));
	}
}

void TaskManager::checkTasks(uint64_t now) {
	std::lock_guard<std::mutex> lock(m_mutex);
	for (auto* t: m_tasks) {
		if (t->isRunning() && t->isWatchdogEnabled()) {
			if ((int64_t)(now - t->getLastHeartbeat()) >
				(int64_t)t->getWatchdogTimeout()) {
				if (t->getRestartPolicy() == Task::RestartPolicy::REBOOT_SYSTEM)
					esp_restart();
				else if (t->getRestartPolicy() == Task::RestartPolicy::RESTART_TASK) {
					t->stop();
					t->start();
				}
			}
		}
	}
}

bool TaskManager::checkHeapIntegrity() {
	bool const ok = heap_caps_check_integrity_all(true);
	if (!ok) {
	}
	return ok;
}

void TaskManager::printTasks() {
	std::lock_guard<std::mutex> lock(m_mutex);
	for (auto* t: m_tasks) {
		uint32_t hwm = t->getStackHighWaterMark();
		Log::debug(TM_TAG, "Task: %s, Stack HWM: %u", t->getName().c_str(), (unsigned int)hwm);
	}
}

} // namespace System
