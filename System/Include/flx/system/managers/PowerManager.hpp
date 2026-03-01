#pragma once

#include <flx/core/Observable.hpp>
#include <flx/core/Singleton.hpp>
#include <flx/hal/power/IPowerDevice.hpp>
#include <flx/services/IService.hpp>
#include <flx/services/ServiceManifest.hpp>
#include <memory>

namespace flx::system {

/**
 * @brief Manages battery levels, charging status, and power-related subjects.
 *
 * Sources data from the HAL IPowerDevice (via DeviceRegistry) when available,
 * falling back to SystemInfoService when no HAL power device is registered.
 * Publishes battery state through LVGL-compatible observables.
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
	 * @brief Refresh battery statistics.
	 * Reads from HAL IPowerDevice if available, else falls back to SystemInfoService.
	 */
	void refresh();

	// Headless-compatible observables
	flx::Observable<int32_t>& getBatteryLevelObservable() { return m_batteryLevel; }
	flx::Observable<int32_t>& getIsChargingObservable() { return m_isCharging; }
	flx::Observable<int32_t>& getIsConfiguredObservable() { return m_isConfigured; }

private:

	PowerManager() = default;
	~PowerManager() = default;

	/// Cached HAL power device (resolved once on start, nullptr if unavailable)
	std::shared_ptr<flx::hal::power::IPowerDevice> m_powerDevice;

	/// HAL power event subscription ID (-1 if not subscribed)
	int m_halSubscriptionId = -1;

	flx::Observable<int32_t> m_batteryLevel {100};
	flx::Observable<int32_t> m_isCharging {0};
	flx::Observable<int32_t> m_isConfigured {0};
};

} // namespace flx::system
