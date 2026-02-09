#pragma once

#include "core/common/Observable.hpp"
#include "core/common/Singleton.hpp"
#include "esp_err.h"
#include "esp_timer.h"
#include "wear_levelling.h"
#include <memory>

#if !CONFIG_FLXOS_HEADLESS_MODE
#include "core/ui/LvglObserverBridge.hpp"
#include "lvgl.h"
#endif

namespace System {
class SystemManager : public Singleton<SystemManager> {
	friend class Singleton<SystemManager>;

public:

	esp_err_t initHardware();
	esp_err_t initServices(); // Headless-compatible: connectivity, settings

#if !CONFIG_FLXOS_HEADLESS_MODE
	esp_err_t initGuiState(); // GUI-only: LVGL subjects, observers, themes
#endif

	wl_handle_t getSystemWlHandle() const { return m_wl_handle_system; }
	wl_handle_t getDataWlHandle() const { return m_wl_handle_data; }

	bool isSafeMode() const { return m_isSafeMode; }

	Observable<int32_t>& getUptimeObservable() { return m_uptime_subject; }

#if !CONFIG_FLXOS_HEADLESS_MODE
	// GUI-only: LVGL subject accessors
	lv_subject_t& getUptimeSubject() { return *m_uptime_bridge->getSubject(); }
#endif

private:

	SystemManager() = default;
	~SystemManager() = default;

	esp_err_t mountStorage();
	static void mount_storage_helper(const char* path, const char* partition_label, wl_handle_t* wl_handle, bool format_if_failed);

	wl_handle_t m_wl_handle_system = WL_INVALID_HANDLE;
	wl_handle_t m_wl_handle_data = WL_INVALID_HANDLE;
	bool m_isSafeMode = false;

	// System state
	Observable<int32_t> m_uptime_subject {0};

#if !CONFIG_FLXOS_HEADLESS_MODE
	std::unique_ptr<LvglObserverBridge<int32_t>> m_uptime_bridge {};
#endif
};

} // namespace System
