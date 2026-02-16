#pragma once

#include "esp_err.h"
#include "esp_timer.h"
#include "wear_levelling.h"
#include <flx/core/Observable.hpp>
#include <flx/core/Singleton.hpp>
#include <memory>

namespace flx::system {

class SystemManager : public flx::Singleton<SystemManager> {
public:

	friend class flx::Singleton<SystemManager>;

	esp_err_t initHardware();
	esp_err_t initServices();

	flx::Observable<int32_t>& getUptimeObservable() { return m_uptime_subject; }
	bool isSafeMode() const { return m_isSafeMode; }

private:

	SystemManager() = default;
	~SystemManager() = default;

	static void mount_storage_helper(const char* path, const char* partition_label, wl_handle_t* wl_handle, bool format_if_failed);

	void registerServices();

	wl_handle_t m_wl_handle_system = WL_INVALID_HANDLE;
	wl_handle_t m_wl_handle_data = WL_INVALID_HANDLE;
	bool m_isSafeMode = false;

	// System state
	flx::Observable<int32_t> m_uptime_subject {0};
};

} // namespace flx::system
