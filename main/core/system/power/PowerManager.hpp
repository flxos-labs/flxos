#pragma once

#include "core/common/Observable.hpp"
#include "core/common/Singleton.hpp"
#include "core/services/IService.hpp"
#include "core/services/ServiceManifest.hpp"
#include <memory>

#if !CONFIG_FLXOS_HEADLESS_MODE
#include "core/ui/LvglObserverBridge.hpp"
#endif

namespace System {

/**
 * @brief Manages battery levels, charging status, and power-related subjects.
 * Bridges state from SystemInfoService to LVGL observers.
 */
class PowerManager : public Singleton<PowerManager>, public Services::IService {
	friend class Singleton<PowerManager>;

public:

	// ──── IService manifest ────
	static const Services::ServiceManifest serviceManifest;
	const Services::ServiceManifest& getManifest() const override { return serviceManifest; }

	// ──── IService lifecycle ────
	bool onStart() override;
	void onStop() override;

#if !CONFIG_FLXOS_HEADLESS_MODE
	void onGuiInit() override;
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

	Observable<int32_t> m_batteryLevel {100};
	Observable<int32_t> m_isCharging {0};
	Observable<int32_t> m_isConfigured {0};

#if !CONFIG_FLXOS_HEADLESS_MODE
	std::unique_ptr<LvglObserverBridge<int32_t>> m_batteryLevelBridge {};
	std::unique_ptr<LvglObserverBridge<int32_t>> m_isChargingBridge {};
#endif
};

} // namespace System
