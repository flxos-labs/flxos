#include "CliService.hpp"
#include "core/common/Logger.hpp"
#include "core/services/systeminfo/SystemInfoService.hpp"

#include "esp_console.h"
#include "esp_system.h"
#include "linenoise/linenoise.h"

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
	printf("CPU Frequency:  %lu MHz\n", sysStats.cpuFreqMhz);
	printf("PSRAM Size:     %lu KB\n", memStats.totalPsram / 1024);
	printf("Free Heap:      %lu bytes\n", memStats.freeHeap);
	printf("Min Free Heap:  %lu bytes\n", memStats.minFreeHeap);
	printf("IDF Version:    %s\n", sysStats.idfVersion.c_str());
	printf("================================\n\n");

	return 0;
}

// Command: heap - Display heap memory statistics
static int cmd_heap(int argc, char** argv) {
	auto& sysInfo = Services::SystemInfoService::getInstance();
	auto memStats = sysInfo.getMemoryStats();

	printf("\n=== Heap Memory ===\n");
	printf("Free Heap:      %lu bytes\n", memStats.freeHeap);
	printf("Min Free Heap:  %lu bytes (all-time low)\n", memStats.minFreeHeap);
	printf("Total Heap:     %lu bytes\n", memStats.totalHeap);
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

void CliService::registerCommands() {
	// sysinfo command
	const esp_console_cmd_t sysinfo_cmd = {
		.command = "sysinfo",
		.help = "Display system information",
		.hint = nullptr,
		.func = &cmd_sysinfo,
		.argtable = nullptr,
		.func_w_context = nullptr,
		.context = nullptr
	};
	ESP_ERROR_CHECK(esp_console_cmd_register(&sysinfo_cmd));

	// heap command
	const esp_console_cmd_t heap_cmd = {
		.command = "heap",
		.help = "Display heap memory statistics",
		.hint = nullptr,
		.func = &cmd_heap,
		.argtable = nullptr,
		.func_w_context = nullptr,
		.context = nullptr
	};
	ESP_ERROR_CHECK(esp_console_cmd_register(&heap_cmd));

	// uptime command
	const esp_console_cmd_t uptime_cmd = {
		.command = "uptime",
		.help = "Display system uptime",
		.hint = nullptr,
		.func = &cmd_uptime,
		.argtable = nullptr,
		.func_w_context = nullptr,
		.context = nullptr
	};
	ESP_ERROR_CHECK(esp_console_cmd_register(&uptime_cmd));

	// reboot command
	const esp_console_cmd_t reboot_cmd = {
		.command = "reboot",
		.help = "Restart the system",
		.hint = nullptr,
		.func = &cmd_reboot,
		.argtable = nullptr,
		.func_w_context = nullptr,
		.context = nullptr
	};
	ESP_ERROR_CHECK(esp_console_cmd_register(&reboot_cmd));

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
