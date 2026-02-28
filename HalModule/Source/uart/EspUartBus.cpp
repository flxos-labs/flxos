#include <driver/gpio.h>
#include <driver/uart.h>
#include <flx/core/Logger.hpp>
#include <flx/hal/uart/EspUartBus.hpp>

namespace flx::hal::uart {

static constexpr std::string_view TAG = "EspUartBus";

EspUartBus::EspUartBus(int port, int txPin, int rxPin, uint32_t rxBufferSize, uint32_t txBufferSize)
	: m_port(port), m_txPin(txPin), m_rxPin(rxPin), m_rxBufferSize(rxBufferSize), m_txBufferSize(txBufferSize) {
	this->setState(State::Uninitialized);
}

EspUartBus::~EspUartBus() {
	if (getState() == State::Ready || m_isOpen) {
		stop();
	}
}

bool EspUartBus::start() {
	this->setState(State::Ready);
	return true; // Actual initialization happens in open()
}

bool EspUartBus::stop() {
	close();
	this->setState(State::Stopped);
	return true;
}

bool EspUartBus::setBaudRate(uint32_t baudRate) {
	std::lock_guard<std::mutex> lock(m_mutex);
	if (!m_isOpen) return false;

	esp_err_t err = uart_set_baudrate(static_cast<uart_port_t>(m_port), baudRate);
	return err == ESP_OK;
}

bool EspUartBus::open(uint32_t baudRate) {
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_isOpen) {
		flx::Log::warn(TAG, "UART %d is already open", m_port);
		return true;
	}

	uart_config_t uart_config = {};
	uart_config.baud_rate = baudRate;
	uart_config.data_bits = UART_DATA_8_BITS;
	uart_config.parity = UART_PARITY_DISABLE;
	uart_config.stop_bits = UART_STOP_BITS_1;
	uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
	uart_config.source_clk = UART_SCLK_DEFAULT;

	uart_port_t port = static_cast<uart_port_t>(m_port);

	esp_err_t err = uart_param_config(port, &uart_config);
	if (err != ESP_OK) {
		flx::Log::error(TAG, "uart_param_config failed: %s", esp_err_to_name(err));
		return false;
	}

	err = uart_set_pin(port, m_txPin, m_rxPin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
	if (err != ESP_OK) {
		flx::Log::error(TAG, "uart_set_pin failed: %s", esp_err_to_name(err));
		return false;
	}

	err = uart_driver_install(port, m_rxBufferSize, m_txBufferSize, 0, nullptr, 0);
	if (err != ESP_OK) {
		flx::Log::error(TAG, "uart_driver_install failed: %s", esp_err_to_name(err));
		return false;
	}

	m_isOpen = true;
	flx::Log::info(TAG, "UART %d opened with baud rate %lu", m_port, baudRate);
	return true;
}

void EspUartBus::close() {
	std::lock_guard<std::mutex> lock(m_mutex);
	if (m_isOpen) {
		uart_driver_delete(static_cast<uart_port_t>(m_port));
		m_isOpen = false;
		flx::Log::info(TAG, "UART %d closed", m_port);
	}
}

size_t EspUartBus::write(const uint8_t* data, size_t len, uint32_t timeoutMs) {
	// Wait on mutex is optional depending on concurrency, but we'll lock to be safe
	std::lock_guard<std::mutex> lock(m_mutex);
	if (!m_isOpen) return 0;

	int written = uart_write_bytes(static_cast<uart_port_t>(m_port), reinterpret_cast<const char*>(data), len);

	if (written < 0) return 0;

	if (timeoutMs > 0) {
		uart_wait_tx_done(static_cast<uart_port_t>(m_port), pdMS_TO_TICKS(timeoutMs));
	}

	return static_cast<size_t>(written);
}

size_t EspUartBus::read(uint8_t* data, size_t maxLen, uint32_t timeoutMs) {
	std::lock_guard<std::mutex> lock(m_mutex);
	if (!m_isOpen) return 0;

	// In read, we shouldn't lock the mutex for long blocking operations,
	// but we'll assume timeoutMs is manageable or the bus is dedicated.
	int read_bytes = uart_read_bytes(static_cast<uart_port_t>(m_port), data, maxLen, pdMS_TO_TICKS(timeoutMs));

	if (read_bytes < 0) return 0;
	return static_cast<size_t>(read_bytes);
}

size_t EspUartBus::available() const {
	if (!m_isOpen) return 0;

	size_t size = 0;
	esp_err_t err = uart_get_buffered_data_len(static_cast<uart_port_t>(m_port), &size);

	if (err == ESP_OK) {
		return size;
	}
	return 0;
}

void EspUartBus::flush() {
	std::lock_guard<std::mutex> lock(m_mutex);
	if (m_isOpen) {
		uart_flush(static_cast<uart_port_t>(m_port));
	}
}

} // namespace flx::hal::uart
