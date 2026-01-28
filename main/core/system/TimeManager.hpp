#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdint>
#include <ctime>

namespace System {

class TimeManager {
public:

	static TimeManager& getInstance();

	void init();
	void deinit();
	void syncTime();
	void setTimeZone(const char* tz);

	bool isSynced() const { return m_is_synced; }
	bool waitForSync(uint32_t timeout_ms = 10000);

	void updateSyncStatus(bool synced);

private:

	TimeManager() = default;
	~TimeManager() = default;
	TimeManager(const TimeManager&) = delete;
	TimeManager& operator=(const TimeManager&) = delete;

	bool m_is_init = false;
	bool m_is_synced = false;
};

} // namespace System
