#include <flx/hal/gps/UartGpsDevice.hpp>
#include <flx/core/Logger.hpp>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace flx::hal::gps {

static constexpr std::string_view TAG = "GpsProbe";

std::shared_ptr<UartGpsDevice> ProbeGpsDevice(std::shared_ptr<flx::hal::uart::IUartBus> uartBus) {
	if (!uartBus) return nullptr;

	const uint32_t bauds[] = {9600, 38400, 115200, 4800};
	bool found = false;
	uint32_t foundBaud = 0;

	for (uint32_t baud : bauds) {
		flx::Log::info(TAG, "Probing GPS at %lu baud...", baud);
		uartBus->open(baud);
		uartBus->setBaudRate(baud); // Ensure it's set
		uartBus->flush();

		// Wait and listen for NMEA traffic
		bool trafficDetected = false;
		for (int attempts = 0; attempts < 20; ++attempts) {
			uint8_t buf[64];
			size_t len = uartBus->read(buf, sizeof(buf) - 1, 100);
			if (len > 0) {
				buf[len] = '\0';
				if (strstr(reinterpret_cast<char*>(buf), "$G") != nullptr) {
					trafficDetected = true;
					break;
				}
			}
		}

		if (trafficDetected) {
			found = true;
			foundBaud = baud;
			flx::Log::info(TAG, "GPS module detected at %lu baud", baud);
			break;
		}

		uartBus->close();
	}

	if (found) {
		auto device = std::make_shared<UartGpsDevice>(uartBus);
		// It will initialize and start RX task on the already-configured baud rate
		device->setModel("Generic NMEA (Probed)");
		return device;
	}

	flx::Log::warn(TAG, "No GPS module detected on UART");
	return nullptr;
}

} // namespace flx::hal::gps
