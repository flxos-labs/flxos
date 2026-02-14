#pragma once

#include <flx/core/Observable.hpp>
#include <flx/core/Singleton.hpp>
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
class PowerManager : public flx::Singleton<PowerManager>, public Services::IService {
	friend class flx::Singleton<PowerManager>;

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
	flx::Observable<int32_t>& getBatteryLevelObservable() { return m_batteryLevel; }
	flx::Observable<int32_t>& getIsChargingObservable() { return m_isCharging; }
	flx::Observable<int32_t>& getIsConfiguredObservable() { return m_isConfigured; }

#if !CONFIG_FLXOS_HEADLESS_MODE
	// LVGL subject accessors
	lv_subject_t& getBatteryLevelSubject() { return *m_batteryLevelBridge->getSubject(); }
	lv_subject_t& getIsChargingSubject() { return *m_isChargingBridge->getSubject(); }
#endif

private:

	PowerManager() = default;
	~PowerManager() = default;

	flx::Observable<int32_t> m_batteryLevel {100};
	flx::Observable<int32_t> m_isCharging {0};
	flx::Observable<int32_t> m_isConfigured {0};

#if !CONFIG_FLXOS_HEADLESS_MODE
	std::unique_ptr<LvglObserverBridge<int32_t>> m_batteryLevelBridge {};
	std::unique_ptr<LvglObserverBridge<int32_t>> m_isChargingBridge {};
#endif
};

} // namespace System
