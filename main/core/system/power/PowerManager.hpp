#pragma once

#include "core/common/Observable.hpp"
#include <memory>

#if !CONFIG_FLXOS_HEADLESS_MODE
#include "core/ui/LvglObserverBridge.hpp"
#endif

namespace System {

/**
 * @brief Manages battery levels, charging status, and power-related subjects.
 * Bridges state from SystemInfoService to LVGL observers.
 */
class PowerManager {
public:

	static PowerManager& getInstance();

	/**
	 * @brief Initialize PowerManager
	 */
	void init();

#if !CONFIG_FLXOS_HEADLESS_MODE
	/**
	 * @brief Initialize LVGL bridges for GUI mode
	 */
	void initGuiBridges();
#endif

	/**
	 * @brief Refresh battery statistics from SystemInfoService
	 */
	void refresh();

	// Headless-compatible observables
	Observable<int32_t>& getBatteryLevelObservable() { return m_batteryLevel; }
	Observable<int32_t>& getIsChargingObservable() { return m_isCharging; }
	Observable<int32_t>& getIsConfiguredObservable() { return m_isConfigured; }

#if !CONFIG_FLXOS_HEADLESS_MODE
	// LVGL subject accessors
	lv_subject_t& getBatteryLevelSubject() { return *m_batteryLevelBridge->getSubject(); }
	lv_subject_t& getIsChargingSubject() { return *m_isChargingBridge->getSubject(); }
#endif

private:

	PowerManager() = default;
	~PowerManager() = default;
	PowerManager(const PowerManager&) = delete;
	PowerManager& operator=(const PowerManager&) = delete;

	Observable<int32_t> m_batteryLevel {100};
	Observable<int32_t> m_isCharging {0};
	Observable<int32_t> m_isConfigured {0};

#if !CONFIG_FLXOS_HEADLESS_MODE
	std::unique_ptr<LvglObserverBridge<int32_t>> m_batteryLevelBridge {};
	std::unique_ptr<LvglObserverBridge<int32_t>> m_isChargingBridge {};
#endif

	bool m_is_init = false;
};

} // namespace System
