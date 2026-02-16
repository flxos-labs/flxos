#pragma once

#include <flx/core/Observable.hpp>
#include <flx/core/Singleton.hpp>
#include <flx/services/IService.hpp>
#include <flx/services/ServiceManifest.hpp>
#include <memory>

namespace flx::system {

/**
 * @brief Manages battery levels, charging status, and power-related subjects.
 * Bridges state from SystemInfoService to LVGL observers.
 */
class PowerManager : public flx::Singleton<PowerManager>, public flx::services::IService {
	friend class flx::Singleton<PowerManager>;

public:

	// ──── IService manifest ────
	static const flx::services::ServiceManifest serviceManifest;
	const flx::services::ServiceManifest& getManifest() const override { return serviceManifest; }

	// ──── IService lifecycle ────
	bool onStart() override;
	void onStop() override;

	/**
	 * @brief Refresh battery statistics from SystemInfoService
	 */
	void refresh();

	// Headless-compatible observables
	flx::Observable<int32_t>& getBatteryLevelObservable() { return m_batteryLevel; }
	flx::Observable<int32_t>& getIsChargingObservable() { return m_isCharging; }
	flx::Observable<int32_t>& getIsConfiguredObservable() { return m_isConfigured; }

private:

	PowerManager() = default;
	~PowerManager() = default;

	flx::Observable<int32_t> m_batteryLevel {100};
	flx::Observable<int32_t> m_isCharging {0};
	flx::Observable<int32_t> m_isConfigured {0};
};

} // namespace flx::system
