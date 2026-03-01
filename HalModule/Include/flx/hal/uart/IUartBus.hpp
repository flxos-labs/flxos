#pragma once

#include <cstddef>
#include <cstdint>
#include <flx/hal/IDevice.hpp>

namespace flx::hal::uart {

/**
 * @brief Abstract interface for a UART bus controller.
 *
 * Surpasses Tactility's C-kernel shim. Brings UART communication natively into
 * the FlxOS HAL as a discoverable, observable device registry citizen.
 * Useful for modems, GPS receivers, custom peripherals.
 */
class IUartBus : public flx::hal::IDevice {
public:

	// ── IDevice ───────────────────────────────────────────────────────────
	Type getType() const override { return Type::Uart; }

	// ── Port access ───────────────────────────────────────────────────────
	/** The active hardware UART port (UART_NUM_0, UART_NUM_1, etc.). */
	virtual int getPort() const = 0;

	// ── Configuration ─────────────────────────────────────────────────────
	/** Change baud rate at runtime. The bus must be open. */
	virtual bool setBaudRate(uint32_t baudRate) = 0;

	/** Open the bus (allocate UART driver if not already done). */
	virtual bool open(uint32_t baudRate) = 0;

	/** Close the bus. */
	virtual void close() = 0;

	// ── Transfers ─────────────────────────────────────────────────────────
	/**
     * @brief Write data blocking or semi-blocking.
     * @param data      Source buffer.
     * @param len       Bytes to write.
     * @param timeoutMs Maximum wait time (use max for blocking).
     * @return Number of chunks successfully written, or 0 on timeout.
     */
	virtual size_t write(const uint8_t* data, size_t len, uint32_t timeoutMs = 1000) = 0;

	/**
     * @brief Read data into buffer. Returns immediately if data is ready.
     * @param data      Destination buffer.
     * @param maxLen    Buffer size.
     * @param timeoutMs Timeout to wait for data.
     * @return Number of bytes actually read.
     */
	virtual size_t read(uint8_t* data, size_t maxLen, uint32_t timeoutMs = 0) = 0;

	/** Check if data is pending in RX ringbuffer without consuming. */
	virtual size_t available() const = 0;

	/** Empty RX and TX buffers. */
	virtual void flush() = 0;
};

} // namespace flx::hal::uart
