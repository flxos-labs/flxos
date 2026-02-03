#include "CliService.hpp"
#include "core/common/Logger.hpp"
#include "core/services/system_info/SystemInfoService.hpp"

#include "esp_console.h"
#include "esp_system.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

static constexpr const char* TAG = "CLI";

namespace System {

CliService& CliService::getInstance() {
	static CliService instance;
	return instance;
}

// Command: sysinfo - Display system information
static int cmd_sysinfo(int argc, char** argv) {
	auto& sysInfo = Services::SystemInfoService::getInstance();
	auto sysStats = sysInfo.getSystemStats();
	auto memStats = sysInfo.getMemoryStats();

	printf("\n=== FlxOS System Information ===\n");
	printf("Chip Model:     %s\n", sysStats.chipModel.c_str());
	printf("Chip Revision:  %d\n", sysStats.revision);
	printf("CPU Cores:      %d\n", sysStats.cores);
	printf("CPU Frequency:  %u MHz\n", (unsigned int)sysStats.cpuFreqMhz);
	printf("PSRAM Size:     %u KB\n", (unsigned int)(memStats.totalPsram / 1024));
	printf("Free Heap:      %u bytes\n", (unsigned int)memStats.freeHeap);
	printf("Min Free Heap:  %u bytes\n", (unsigned int)memStats.minFreeHeap);
	printf("IDF Version:    %s\n", sysStats.idfVersion.c_str());
	printf("================================\n\n");

	return 0;
}

// Command: heap - Display heap memory statistics
static int cmd_heap(int argc, char** argv) {
	auto& sysInfo = Services::SystemInfoService::getInstance();
	auto memStats = sysInfo.getMemoryStats();

	printf("\n=== Heap Memory ===\n");
	printf("Free Heap:      %u bytes\n", (unsigned int)memStats.freeHeap);
	printf("Min Free Heap:  %u bytes (all-time low)\n", (unsigned int)memStats.minFreeHeap);
	printf("Total Heap:     %u bytes\n", (unsigned int)memStats.totalHeap);
	printf("===================\n\n");

	return 0;
}

// Command: uptime - Display system uptime
static int cmd_uptime(int argc, char** argv) {
	int64_t uptime_us = esp_timer_get_time();
	int64_t uptime_sec = uptime_us / 1000000;

	int days = uptime_sec / 86400;
	int hours = (uptime_sec % 86400) / 3600;
	int minutes = (uptime_sec % 3600) / 60;
	int seconds = uptime_sec % 60;

	printf("\nUptime: ");
	if (days > 0) {
		printf("%d day%s, ", days, days > 1 ? "s" : "");
	}
	printf("%02d:%02d:%02d\n\n", hours, minutes, seconds);

	return 0;
}

// Command: reboot - Restart the system
static int cmd_reboot(int argc, char** argv) {
	printf("\nRebooting system...\n");
	vTaskDelay(pdMS_TO_TICKS(500)); // Brief delay for message to flush
	esp_restart();
	return 0; // Never reached
}

#define REGISTER_CLI_CMD(name, help_text, handler)       \
	{                                                    \
		const esp_console_cmd_t cmd = {                  \
			.command = name,                             \
			.help = help_text,                           \
			.hint = nullptr,                             \
			.func = handler,                             \
			.argtable = nullptr,                         \
			.func_w_context = nullptr,                   \
			.context = nullptr                           \
		};                                               \
		ESP_ERROR_CHECK(esp_console_cmd_register(&cmd)); \
	}

void CliService::registerCommands() {
	REGISTER_CLI_CMD("sysinfo", "Display system information", &cmd_sysinfo);
	REGISTER_CLI_CMD("heap", "Display heap memory statistics", &cmd_heap);
	REGISTER_CLI_CMD("uptime", "Display system uptime", &cmd_uptime);
	REGISTER_CLI_CMD("reboot", "Restart the system", &cmd_reboot);

	Log::info(TAG, "Registered CLI commands: sysinfo, heap, uptime, reboot");
}

esp_err_t CliService::init() {
	if (m_running) {
		return ESP_OK;
	}

	Log::info(TAG, "Initializing CLI...");

	// Initialize console
	esp_console_repl_t* repl = nullptr;
	esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
	repl_config.prompt = "flxos> ";
	repl_config.max_cmdline_length = 256;

	// Register help command
	esp_console_register_help_command();

	// Register our custom commands
	registerCommands();

	// Configure UART for console
	esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));

	// Start REPL
	ESP_ERROR_CHECK(esp_console_start_repl(repl));

	m_running = true;
	Log::info(TAG, "CLI started. Type 'help' for available commands.");

	return ESP_OK;
}

} // namespace System
