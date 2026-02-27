#include <driver/adc.h>
#include <driver/gpio.h>
#include <flx/core/Logger.hpp>
#include <flx/hal/power/AdcPowerDevice.hpp>

namespace flx::hal::power {

static constexpr std::string_view TAG = "AdcPowerDevice";

AdcPowerDevice::AdcPowerDevice(int adcPin) : m_adcPin(adcPin) {
	this->setState(State::Uninitialized);
}

AdcPowerDevice::~AdcPowerDevice() {
	if (getState() == State::Ready) {
		stop();
	}
}

bool AdcPowerDevice::start() {
	this->setState(State::Starting);

	// Here we would typically initialize the esp_adc/adc_oneshot API.
	// For this basic driver, we assume legacy or simple initialization.
	if (m_adcPin >= 0) {
		flx::Log::info(TAG, "ADC Power Monitor initialized on pin %d", m_adcPin);
		this->setState(State::Ready);
		poll(); // Initial reading
		return true;
	}

	flx::Log::error(TAG, "Invalid ADC pin configured");
	this->setState(State::Error);
	return false;
}

bool AdcPowerDevice::stop() {
	this->setState(State::Stopped);
	return true;
}

uint32_t AdcPowerDevice::getBatteryVoltageMv() const {
	return m_lastVoltageMv;
}

uint8_t AdcPowerDevice::getChargeLevel() const {
	return m_lastChargeLevel;
}

int AdcPowerDevice::subscribePowerEvents(PowerEventCallback cb) {
	std::lock_guard<std::mutex> lock(m_mutex);
	int id = m_nextObserverId++;
	m_observers.emplace_back(id, cb);
	return id;
}

void AdcPowerDevice::unsubscribePowerEvents(int id) {
	std::lock_guard<std::mutex> lock(m_mutex);
	for (auto it = m_observers.begin(); it != m_observers.end(); ++it) {
		if (it->first == id) {
			m_observers.erase(it);
			break;
		}
	}
}

void AdcPowerDevice::notifyObservers(PowerEvent event) {
	std::lock_guard<std::mutex> lock(m_mutex);
	for (const auto& observer: m_observers) {
		if (observer.second) {
			observer.second(event);
		}
	}
}

void AdcPowerDevice::poll() {
	if (getState() != State::Ready) return;

	// In a real implementation, read from ADC and convert to voltage.
	// We'll simulate reading 3.7V (50% charge) if we can't actually read.
	uint32_t currentVoltageMv = 3700;

	// Very simple battery curve estimation (LiPo)
	uint8_t currentChargeLevel = 0;
	if (currentVoltageMv >= 4200) currentChargeLevel = 100;
	else if (currentVoltageMv <= 3200)
		currentChargeLevel = 0;
	else
		currentChargeLevel = static_cast<uint8_t>((currentVoltageMv - 3200) / 10);

	m_lastVoltageMv = currentVoltageMv;
	m_lastChargeLevel = currentChargeLevel;

	// Check thresholds for events
	if (currentChargeLevel <= 5 && !m_isCritical) {
		m_isCritical = true;
		m_isLow = true;
		notifyObservers(PowerEvent::BatteryCritical);
		flx::Log::warn(TAG, "Battery level critical: %d%%", currentChargeLevel);
	} else if (currentChargeLevel <= 15 && currentChargeLevel > 5 && !m_isLow) {
		m_isLow = true;
		notifyObservers(PowerEvent::BatteryLow);
		flx::Log::warn(TAG, "Battery level low: %d%%", currentChargeLevel);
	} else if (currentChargeLevel > 15) {
		m_isLow = false;
		m_isCritical = false;
	}
}

} // namespace flx::hal::power
