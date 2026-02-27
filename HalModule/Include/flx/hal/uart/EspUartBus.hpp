#pragma once

#include <flx/hal/DeviceBase.hpp>
#include <flx/hal/uart/IUartBus.hpp>
#include <mutex>
#include <string_view>

namespace flx::hal::uart {

class EspUartBus : public DeviceBase<IUartBus> {
public:
	EspUartBus(int port, int txPin, int rxPin, uint32_t rxBufferSize = 1024, uint32_t txBufferSize = 1024);
	~EspUartBus() override;

	// ── IDevice ───────────────────────────────────────────────────────────
	std::string_view getName() const override { return "ESP UART Bus"; }
	std::string_view getDescription() const override { return "ESP-IDF UART Controller"; }
	bool start() override;
	bool stop() override;
	Type getType() const override { return IUartBus::getType(); }

	// ── IUartBus ──────────────────────────────────────────────────────────
	int getPort() const override { return m_port; }
	bool setBaudRate(uint32_t baudRate) override;
	bool open(uint32_t baudRate) override;
	void close() override;

	size_t write(const uint8_t* data, size_t len, uint32_t timeoutMs = 1000) override;
	size_t read(uint8_t* data, size_t maxLen, uint32_t timeoutMs = 0) override;
	size_t available() const override;
	void flush() override;

private:
	int m_port;
	int m_txPin;
	int m_rxPin;
	uint32_t m_rxBufferSize;
	uint32_t m_txBufferSize;
	bool m_isOpen = false;
	std::mutex m_mutex;
};

} // namespace flx::hal::uart
