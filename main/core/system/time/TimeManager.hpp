#pragma once

#include "core/common/Singleton.hpp"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdint>
#include <ctime>

namespace System {

class TimeManager : public Singleton<TimeManager> {
	friend class Singleton<TimeManager>;

public:

	void init();
	void deinit();
	void syncTime();
	static void setTimeZone(const char* tz);

	bool isSynced() const { return m_is_synced; }
	bool waitForSync(uint32_t timeout_ms = 10000);

	void updateSyncStatus(bool synced);

private:

	TimeManager() = default;
	~TimeManager() = default;

	bool m_is_init = false;
	bool m_is_synced = false;

	void setCompileTime();
};

} // namespace System
