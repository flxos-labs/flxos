#pragma once

#include <flx/core/Singleton.hpp>
#include "core/services/IService.hpp"
#include "core/services/ServiceManifest.hpp"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdint>
#include <ctime>

namespace System {

class TimeManager : public flx::Singleton<TimeManager>, public Services::IService {
	friend class flx::Singleton<TimeManager>;

public:

	// ──── IService manifest ────
	static const Services::ServiceManifest serviceManifest;
	const Services::ServiceManifest& getManifest() const override { return serviceManifest; }

	// ──── IService lifecycle ────
	bool onStart() override;
	void onStop() override;

	void syncTime();
	static void setTimeZone(const char* tz);

	bool isSynced() const { return m_is_synced; }
	bool waitForSync(uint32_t timeout_ms = 10000);

	void updateSyncStatus(bool synced);

private:

	TimeManager() = default;
	~TimeManager() = default;

	bool m_is_synced = false;

	void setCompileTime();
};

} // namespace System
