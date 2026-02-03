#pragma once

#include "esp_err.h"

namespace System {

/**
 * @brief CLI Service for headless mode operation
 *
 * Provides an interactive command-line interface via serial console
 * using ESP-IDF's esp_console component. Designed for future GUI integration
 * where commands could be sent via other interfaces (e.g., serial terminal widget).
 */
class CliService {
public:

	static CliService& getInstance();

	/**
	 * @brief Initialize and start the CLI REPL
	 * @return ESP_OK on success
	 */
	esp_err_t init();

	/**
	 * @brief Check if CLI is running
	 */
	bool isRunning() const { return m_running; }

private:

	CliService() = default;
	~CliService() = default;
	CliService(const CliService&) = delete;
	CliService& operator=(const CliService&) = delete;

	void registerCommands();

	bool m_running = false;
};

} // namespace System
