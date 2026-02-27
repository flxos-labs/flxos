#pragma once

#include <flx/hal/DeviceBase.hpp>
#include <flx/hal/power/IPowerDevice.hpp>
#include <mutex>
#include <vector>

namespace flx::hal::power {

/**
 * @brief Basic ADC-based battery monitor.
 * Polls an analog pin to estimate battery voltage and percentage.
 */
class AdcPowerDevice : public DeviceBase<IPowerDevice> {
public:

	explicit AdcPowerDevice(int adcPin);
	~AdcPowerDevice() override;

	// ── IDevice ───────────────────────────────────────────────────────────
	std::string_view getName() const override { return "ADC Battery Monitor"; }
	std::string_view getDescription() const override { return "ESP32 ADC-based basic battery monitor"; }
	bool start() override;
	bool stop() override;
	Type getType() const override { return IPowerDevice::getType(); }

	// ── IPowerDevice ──────────────────────────────────────────────────────
	bool isCharging() const override { return false; } // Basic ADC cannot detect charging state reliably
	int32_t getCurrentMa() const override { return 0; } // Unsupported
	uint32_t getBatteryVoltageMv() const override;
	uint8_t getChargeLevel() const override;

	int subscribePowerEvents(PowerEventCallback cb) override;
	void unsubscribePowerEvents(int id) override;

	// ── Polling ───────────────────────────────────────────────────────────
	/**
     * @brief Poll the ADC to update internal state.
     * Should be called periodically by a background task.
     * Triggers PowerEvents if thresholds are crossed.
     */
	void poll();

private:

	int m_adcPin;
	uint32_t m_lastVoltageMv = 0;
	uint8_t m_lastChargeLevel = 0;
	bool m_isLow = false;
	bool m_isCritical = false;

	std::mutex m_mutex;
	std::vector<std::pair<int, PowerEventCallback>> m_observers;
	int m_nextObserverId = 1;

	void notifyObservers(PowerEvent event);
};

} // namespace flx::hal::power
