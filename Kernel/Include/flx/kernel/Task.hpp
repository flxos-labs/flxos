#pragma once

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <atomic>
#include <string>

namespace flx::kernel {

class Task {
public:

	enum class RestartPolicy {
		NEVER,
		RESTART_TASK, // WARNING: Forcefully restarting a task can orphan mutexes
		// and leak memory
		REBOOT_SYSTEM
	};

	Task(const std::string& name, uint32_t stackSize, UBaseType_t priority, BaseType_t coreId = tskNO_AFFINITY);
	virtual ~Task();

	bool start(void* data = nullptr);
	void stop();
	void requestStop() { m_stopRequested = true; }
	void suspend() {
		TaskHandle_t h = m_handle.load();
		if (h) {
#if INCLUDE_vTaskSuspend
			vTaskSuspend(h);
#endif
		}
	}
	void resume() {
		TaskHandle_t h = m_handle.load();
		if (h) {
#if INCLUDE_vTaskSuspend
			vTaskResume(h);
#endif
		}
	}
	void join() const;

	bool shouldStop() const { return m_stopRequested.load(); }

	void heartbeat() { m_lastHeartbeat = esp_timer_get_time() / 1000; }
	void setWatchdogTimeout(uint32_t timeoutMs) {
		m_watchdogTimeoutMs = timeoutMs;
	}
	uint32_t getWatchdogTimeout() const { return m_watchdogTimeoutMs; }
	uint64_t getLastHeartbeat() const { return m_lastHeartbeat; }
	bool isWatchdogEnabled() const { return m_watchdogTimeoutMs > 0; }

	uint32_t getStackHighWaterMark() {
		TaskHandle_t h = m_handle.load();
#if INCLUDE_uxTaskGetStackHighWaterMark
		return h ? uxTaskGetStackHighWaterMark(h) : 0;
#else
		return 0;
#endif
	}
	uint32_t getStackSize() const { return m_stackSize; }

	void setRestartPolicy(RestartPolicy policy) { m_restartPolicy = policy; }
	RestartPolicy getRestartPolicy() const { return m_restartPolicy; }

	TaskHandle_t getHandle() const { return m_handle.load(); }
	std::string getName() const { return m_name; }
	bool isRunning() const { return m_handle.load() != nullptr; }

protected:

	virtual void run(void* data) = 0;

private:

	static void taskEntry(void* param);
	std::string m_name {};
	uint32_t m_stackSize;
	UBaseType_t m_priority;
	BaseType_t m_coreId;
	std::atomic<TaskHandle_t> m_handle {nullptr};
	std::atomic<bool> m_stopRequested {false};
	void* m_data = nullptr;
	std::atomic<uint64_t> m_lastHeartbeat {0};
	uint32_t m_watchdogTimeoutMs = 0;
	RestartPolicy m_restartPolicy = RestartPolicy::REBOOT_SYSTEM;
};

} // namespace flx::kernel
