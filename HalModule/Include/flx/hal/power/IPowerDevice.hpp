#pragma once

#include <cstdint>
#include <flx/hal/IDevice.hpp>
#include <functional>

namespace flx::hal::power {

/**
 * @brief Abstract interface for a power management / battery IC device.
 *
 * Implemented by drivers like AXP2101, BQ25896, or simple ADC battery monitors.
 *
 * Key improvements over Tactility's PowerDevice:
 *  - Event-driven: PowerEvent enumerates state changes.
 *  - Subscription pattern: No more polling `isCharging()` in a loop.
 *  - Advanced metrics: getTemperatureCelsius().
 *  - Explicit capability queries: supportsPowerOff(), supportsChargeControl().
 */
class IPowerDevice : public flx::hal::IDevice {
public:

	// ── IDevice ───────────────────────────────────────────────────────────
	Type getType() const override { return Type::Power; }

	// ── Metrics ───────────────────────────────────────────────────────────
	/** true if external power is connected and currently charging battery. */
	virtual bool isCharging() const = 0;

	/** Battery discharge/charge current in mA. Positive = charging. */
	virtual int32_t getCurrentMa() const = 0;

	/** Battery voltage in mV. */
	virtual uint32_t getBatteryVoltageMv() const = 0;

	/** Battery state of charge (SoC) in percent [0–100]. */
	virtual uint8_t getChargeLevel() const = 0;

	// ── Charge control ────────────────────────────────────────────────────
	virtual bool supportsChargeControl() const { return false; }
	virtual void setChargingAllowed(bool allowed) { (void)allowed; }

	// ── Power control ─────────────────────────────────────────────────────
	virtual bool supportsPowerOff() const { return false; }
	/** Instruct the PMIC to cut system power. May never return. */
	virtual void powerOff() {}

	// ── Thermal monitoring ────────────────────────────────────────────────
	virtual bool supportsTemperature() const { return false; }
	virtual float getTemperatureCelsius() const { return 0.0f; }

	// ── Power events ──────────────────────────────────────────────────────
	enum class PowerEvent : uint8_t {
		ChargeStarted,
		ChargeStopped,
		BatteryLow, ///< Battery dropped below 15%
		BatteryCritical, ///< Battery dropped below 5%
		ExternalPowerConnected,
		ExternalPowerDisconnected
	};

	using PowerEventCallback = std::function<void(PowerEvent)>;

	/**
     * @brief Subscribe to power state changes.
     * @return Subscription ID, or -1 if unsupported.
     */
	virtual int subscribePowerEvents(PowerEventCallback cb) {
		(void)cb;
		return -1;
	}
	virtual void unsubscribePowerEvents(int id) { (void)id; }
};

} // namespace flx::hal::power
