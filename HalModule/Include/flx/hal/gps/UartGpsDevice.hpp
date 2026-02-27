#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <flx/hal/DeviceBase.hpp>
#include <flx/hal/gps/IGpsDevice.hpp>
#include <flx/hal/uart/IUartBus.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace flx::hal::gps {

class UartGpsDevice : public DeviceBase<IGpsDevice> {
public:

	explicit UartGpsDevice(std::shared_ptr<flx::hal::uart::IUartBus> uartBus);
	~UartGpsDevice() override;

	// ── IDevice ───────────────────────────────────────────────────────────
	std::string_view getName() const override { return "UART GPS"; }
	std::string_view getDescription() const override { return "NMEA over UART GPS driver"; }
	bool start() override;
	bool stop() override;
	Type getType() const override { return IGpsDevice::getType(); }

	// ── IGpsDevice ────────────────────────────────────────────────────────
	GpsState getGpsState() const override;
	GpsPosition getLastPosition() const override;
	int subscribePosition(PositionCallback cb) override;
	void unsubscribePosition(int id) override;
	std::string_view getGpsModel() const override;

	void requestColdStart() override;

	// ── Internal ──────────────────────────────────────────────────────────
	void setModel(std::string_view model);

private:

	std::shared_ptr<flx::hal::uart::IUartBus> m_uart;
	GpsState m_state = GpsState::Off;
	GpsPosition m_lastPosition;
	std::string m_model = "Unknown";

	mutable std::mutex m_mutex;
	std::vector<std::pair<int, PositionCallback>> m_observers;
	int m_nextObserverId = 1;

	TaskHandle_t m_rxTaskHandle = nullptr;
	bool m_isRunning = false;

	static void rxTaskRunner(void* arg);
	void processIncomingData();
	void parseNmeaSentence(const std::string& sentence);
	void notifyObservers();
};

/**
 * @brief Auto-probes the UART bus to detect and configure a connected GPS module.
 * Scans common baud rates (9600, 38400, 115200) and listens for NMEA sentences.
 */
std::shared_ptr<UartGpsDevice> ProbeGpsDevice(std::shared_ptr<flx::hal::uart::IUartBus> uartBus);

} // namespace flx::hal::gps
