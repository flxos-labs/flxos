#pragma once

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <atomic>
#include <mutex>
#include <string>
#include <vector>

#include "Task.hpp"

namespace flx::kernel {

class TaskManager {
public:

	static TaskManager& getInstance();

	void registerTask(Task* task);
	void unregisterTask(Task* task);
	Task* getTask(const std::string& name);

	void initWatchdog(uint32_t checkIntervalMs = 1000);
	void checkTasks(uint64_t nowMs);
	static bool checkHeapIntegrity();

	void printTasks();

private:

	TaskManager() = default;
	~TaskManager() = default;
	TaskManager(const TaskManager&) = delete;
	TaskManager& operator=(const TaskManager&) = delete;

	static void watchdogTaskEntry(void* param);

	std::vector<Task*> m_tasks {};
	std::mutex m_mutex {};
	TaskHandle_t m_watchdogTaskHandle = nullptr;
	uint32_t m_checkIntervalMs = 1000;
};

} // namespace flx::kernel
