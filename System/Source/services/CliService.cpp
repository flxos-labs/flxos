#include <flx/core/EventBus.hpp>
#include <flx/core/Logger.hpp>
#include <flx/hal/DeviceRegistry.hpp>
#include <flx/hal/i2c/II2cBus.hpp>
#include <flx/system/services/CliService.hpp>
#include <flx/system/services/SystemInfoService.hpp>

#include "esp_console.h"
#include "esp_err.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <string>
#include <vector>

#include <flx/connectivity/ConnectivityManager.hpp>
#include <flx/connectivity/wifi/WiFiManager.hpp>
#include <flx/core/GuiLock.hpp>
#include <flx/system/managers/DisplayManager.hpp>
#include <flx/system/services/FileSystemService.hpp>
#include <sstream>
#include <sys/time.h>
#include <time.h>

static constexpr const char* TAG = "CLI";

// Helper to normalize paths (handle . and ..)
static std::string resolvePath(const std::string& base, const std::string& part) {
	std::string fullPath;
	if (part.empty())
		return base;

	// If part is absolute, ignore base
	if (part[0] == '/') {
		fullPath = part;
	} else {
		fullPath = base;
		if (fullPath.back() != '/')
			fullPath += '/';
		fullPath += part;
	}

	// Tokenize
	std::vector<std::string> parts;
	std::stringstream ss(fullPath);
	std::string token;

	while (std::getline(ss, token, '/')) {
		if (token.empty() || token == ".")
			continue;
		if (token == "..") {
			if (!parts.empty())
				parts.pop_back();
		} else {
			parts.push_back(token);
		}
	}

	// Reconstruct
	if (parts.empty())
		return "/";
	std::string result;
	for (const auto& p: parts) {
		result += "/" + p;
	}
	return result;
}

namespace flx::system {

const flx::services::ServiceManifest CliService::serviceManifest = {
	.serviceId = "com.flxos.cli",
	.serviceName = "CLI",
	.dependencies = {},
	.priority = 90,
	.required = false,
	.autoStart = false,
	.guiRequired = false,
	.capabilities = flx::services::ServiceCapability::None,
	.description = "Interactive serial console CLI",
};

CliService& CliService::getInstance() {
	static CliService instance;
	return instance;
}

// Command: sysinfo - Display system information
static int cmdSysInfo(int /*argc*/, char** /*argv*/) {
	auto& sys_info = flx::services::SystemInfoService::getInstance();
	auto sys_stats = sys_info.getSystemStats();
	auto mem_stats = sys_info.getMemoryStats();

	printf("\n=== FlxOS System Information ===\n");
	printf("Chip Model:     %s\n", sys_stats.chipModel.c_str());
	printf("Chip Revision:  %d\n", sys_stats.revision);
	printf("CPU Cores:      %d\n", sys_stats.cores);
	printf("CPU Frequency:  %u MHz\n", (unsigned int)sys_stats.cpuFreqMhz);
	printf("PSRAM Size:     %u KB\n", (unsigned int)(mem_stats.totalPsram / 1024));
	printf("Free Heap:      %u bytes\n", (unsigned int)mem_stats.freeHeap);
	printf("Min Free Heap:  %u bytes\n", (unsigned int)mem_stats.minFreeHeap);
	printf("IDF Version:    %s\n", sys_stats.idfVersion.c_str());
	printf("================================\n\n");

	return 0;
}

// Command: heap - Display heap memory statistics
static int cmdHeap(int /*argc*/, char** /*argv*/) {
	auto& sys_info = flx::services::SystemInfoService::getInstance();
	auto mem_stats = sys_info.getMemoryStats();

	printf("\n=== Heap Memory ===\n");
	printf("Free Heap:      %u bytes\n", (unsigned int)mem_stats.freeHeap);
	printf("Min Free Heap:  %u bytes (all-time low)\n", (unsigned int)mem_stats.minFreeHeap);
	printf("Total Heap:     %u bytes\n", (unsigned int)mem_stats.totalHeap);
	printf("===================\n\n");

	return 0;
}

// Command: uptime - Display system uptime
static int cmdUptime(int /*argc*/, char** /*argv*/) {
	int64_t const uptime_us = esp_timer_get_time();
	int64_t const uptime_sec = uptime_us / 1000000;

	int const days = uptime_sec / 86400;
	int const hours = (uptime_sec % 86400) / 3600;
	int const minutes = (uptime_sec % 3600) / 60;
	int const seconds = uptime_sec % 60;

	printf("\nUptime: ");
	if (days > 0) {
		printf("%d day%s, ", days, days > 1 ? "s" : "");
	}
	printf("%02d:%02d:%02d\n\n", hours, minutes, seconds);

	return 0;
}

// Command: tasks - List FreeRTOS tasks
static int cmdTasks(int /*argc*/, char** /*argv*/) {
	auto& sys_info = flx::services::SystemInfoService::getInstance();
	auto task_list = sys_info.getTaskList();

	printf("\n=== Task List (%zu tasks) ===\n", task_list.size());
	printf("%-20s %-8s %-12s %-6s %-6s %-6s\n", "Name", "State", "Stack (B)", "Pri", "Core", "CPU %");
	printf("----------------------------------------------------------------\n");

	for (const auto& task: task_list) {
		printf("%-20s %-8s %-12lu %-6d %-6d %-6.1f\n", task.name.c_str(), task.state.c_str(), (unsigned long)task.stackHighWaterMark, task.currentPriority, task.coreID, task.cpuUsagePercent);
	}
	printf("================================================================\n\n");
	return 0;
}

// Command: storage - Display filesystem usage
static int cmdStorage(int /*argc*/, char** /*argv*/) {
	auto& sys_info = flx::services::SystemInfoService::getInstance();
	auto storage_stats = sys_info.getStorageStats();

	printf("\n=== Storage Statistics ===\n");
	if (storage_stats.empty()) {
		printf("No mounted partitions found.\n");
	} else {
		printf("%-10s %-12s %-12s %-12s\n", "Partition", "Total", "Used", "Free");
		printf("--------------------------------------------------\n");
		for (const auto& stat: storage_stats) {
			printf("%-10s %-12s %-12s %-12s\n", stat.name.c_str(), flx::services::SystemInfoService::formatBytes(stat.totalBytes).c_str(), flx::services::SystemInfoService::formatBytes(stat.usedBytes).c_str(), flx::services::SystemInfoService::formatBytes(stat.freeBytes).c_str());
		}
	}
	printf("==========================\n\n");
	return 0;
}

// Command: psram - Display PSRAM statistics
static int cmdPsram(int /*argc*/, char** /*argv*/) {
	auto& sys_info = flx::services::SystemInfoService::getInstance();
	auto mem_stats = sys_info.getMemoryStats();

	printf("\n=== PSRAM Statistics ===\n");
	if (!mem_stats.hasPsram) {
		printf("PSRAM not detected or disabled.\n");
	} else {
		printf("Total PSRAM:    %u bytes (%s)\n", (unsigned int)mem_stats.totalPsram, flx::services::SystemInfoService::formatBytes(mem_stats.totalPsram).c_str());
		printf("Free PSRAM:     %u bytes (%s)\n", (unsigned int)mem_stats.freePsram, flx::services::SystemInfoService::formatBytes(mem_stats.freePsram).c_str());
		printf("Used PSRAM:     %u bytes (%s)\n", (unsigned int)mem_stats.usedPsram, flx::services::SystemInfoService::formatBytes(mem_stats.usedPsram).c_str());
		printf("Usage:          %d%%\n", mem_stats.usagePercentPsram);
	}
	printf("========================\n\n");
	return 0;
}

// Command: version - Display version information
static int cmdVersion(int /*argc*/, char** /*argv*/) {
	auto& sys_info = flx::services::SystemInfoService::getInstance();
	auto sys_stats = sys_info.getSystemStats();

	printf("\n=== Version Information ===\n");
	printf("FlxOS Version:  %s\n", sys_stats.flxosVersion.c_str());
	printf("ESP-IDF Ver:    %s\n", sys_stats.idfVersion.c_str());
	printf("Build Date:     %s\n", sys_stats.buildDate.c_str());
	printf("Boot Reason:    %s\n", sys_stats.bootReason.c_str());
	printf("===========================\n\n");
	return 0;
}

// Command: chip - Display chip information
static int cmdChip(int /*argc*/, char** /*argv*/) {
	auto& sys_info = flx::services::SystemInfoService::getInstance();
	auto sys_stats = sys_info.getSystemStats();

	printf("\n=== Chip Information ===\n");
	printf("Model:          %s\n", sys_stats.chipModel.c_str());
	printf("Revision:       %d\n", sys_stats.revision);
	printf("Cores:          %d\n", sys_stats.cores);
	printf("Features:       %s\n", sys_stats.features.c_str());
	printf("Frequency:      %u MHz\n", (unsigned int)sys_stats.cpuFreqMhz);
	printf("========================\n\n");
	return 0;
}

// Command: wifi - WiFi management
// WiFi Helpers
static int cmdWiFiStatus() {
	auto& wifi_mgr = flx::connectivity::WiFiManager::getInstance();
	auto wifi_stats = flx::services::SystemInfoService::getInstance().getWiFiStats();
	printf("\n=== WiFi Status ===\n");
	printf("State:          %s\n", wifi_mgr.isEnabled() ? "Enabled" : "Disabled");
	printf("Connected:      %s\n", wifi_stats.connected ? "Yes" : "No");
	printf("SSID:           %s\n", wifi_stats.ssid.c_str());
	printf("IP Address:     %s\n", wifi_stats.ipAddress.c_str());
	printf("RSSI:           %d dBm (%s)\n", wifi_stats.rssi, wifi_stats.signalStrength.c_str());
	printf("MAC Address:    %02X:%02X:%02X:%02X:%02X:%02X\n", wifi_stats.mac[0], wifi_stats.mac[1], wifi_stats.mac[2], wifi_stats.mac[3], wifi_stats.mac[4], wifi_stats.mac[5]);
	printf("===================\n\n");
	return 0;
}

static int cmdWiFiOn() {
	flx::connectivity::WiFiManager::getInstance().setEnabled(true);
	printf("WiFi enabled.\n");
	return 0;
}

static int cmdWiFiOff() {
	flx::connectivity::WiFiManager::getInstance().setEnabled(false);
	printf("WiFi disabled.\n");
	return 0;
}

static int cmdWiFiScan() {
	printf("Starting WiFi scan...\n");
	flx::connectivity::WiFiManager::getInstance().scan([](const std::vector<wifi_ap_record_t>& aps) {
		printf("\n=== Scan Results (%zu APs) ===\n", aps.size());
		printf("%-32s %-6s %-6s %-20s\n", "SSID", "RSSI", "Chan", "Auth");
		printf("------------------------------------------------------------------\n");
		for (const auto& ap: aps) {
			// Determine auth mode string
			const char* auth_mode = "OPEN";
			if (ap.authmode == WIFI_AUTH_WEP) auth_mode = "WEP";
			else if (ap.authmode == WIFI_AUTH_WPA_PSK)
				auth_mode = "WPA_PSK";
			else if (ap.authmode == WIFI_AUTH_WPA2_PSK)
				auth_mode = "WPA2_PSK";
			else if (ap.authmode == WIFI_AUTH_WPA_WPA2_PSK)
				auth_mode = "WPA_WPA2_PSK";
			else if (ap.authmode == WIFI_AUTH_WPA3_PSK)
				auth_mode = "WPA3_PSK";

			printf("%-32s %-6d %-6d %-20s\n", ap.ssid, ap.rssi, ap.primary, auth_mode);
		}
		printf("================================\n\nflxos> "); // Reprint prompt
		fflush(stdout);
	});
	return 0;
}

static int cmdWiFiConnect(int argc, char** argv) {
	if (argc < 4) {
		printf("Usage: wifi connect <ssid> <password>\n");
		return 1;
	}
	std::string ssid = argv[2];
	std::string pass = argv[3];
	printf("Connecting to '%s'...\n", ssid.c_str());
	flx::connectivity::WiFiManager::getInstance().connect(ssid.c_str(), pass.c_str());
	return 0;
}

static int cmdWiFiDisconnect() {
	flx::connectivity::WiFiManager::getInstance().disconnect();
	printf("Disconnected from WiFi.\n");
	return 0;
}

static int cmdWiFiHelp(const std::string& subcmd) {
	printf("Unknown subcommand: %s. usage: wifi [status|on|off|scan|connect|disconnect]\n", subcmd.c_str());
	return 1;
}

// Command: wifi - WiFi management
static int cmdWiFi(int argc, char** argv) {
	if (argc == 1 || (argc > 1 && strcmp(argv[1], "status") == 0)) {
		return cmdWiFiStatus();
	}

	std::string subcmd = argv[1];

	if (subcmd == "on") {
		return cmdWiFiOn();
	} else if (subcmd == "off") {
		return cmdWiFiOff();
	} else if (subcmd == "scan") {
		return cmdWiFiScan();
	} else if (subcmd == "connect") {
		return cmdWiFiConnect(argc, argv);
	} else if (subcmd == "disconnect") {
		return cmdWiFiDisconnect();
	} else {
		return cmdWiFiHelp(subcmd);
	}
}

// Command: hotspot - Hotspot management
static int cmdHotspot(int argc, char** argv) {
	auto& conn_mgr = flx::connectivity::ConnectivityManager::getInstance();

	if (argc == 1 || (argc > 1 && strcmp(argv[1], "status") == 0)) {
		bool enabled = conn_mgr.isHotspotEnabled();
		printf("\n=== Hotspot Status ===\n");
		printf("State:          %s\n", enabled ? "Enabled" : "Disabled");
		printf("NAT Enabled:    %s\n", conn_mgr.isHotspotNatEnabled() ? "Yes" : "No");
		printf("======================\n\n");
		return 0;
	}

	std::string subcmd = argv[1];

	if (subcmd == "on") {
		if (conn_mgr.isHotspotEnabled()) {
			printf("Hotspot is already enabled.\n");
		} else {
			// Retrieve stored configuration
			std::string ssid = conn_mgr.getHotspotSsidObservable().get();
			std::string pass = conn_mgr.getHotspotPasswordObservable().get();
			int channel = conn_mgr.getHotspotChannelObservable().get();
			int max_conn = conn_mgr.getHotspotMaxConnObservable().get();
			bool hidden = conn_mgr.getHotspotHiddenObservable().get() != 0;
			int auth_val = conn_mgr.getHotspotAuthObservable().get();
			wifi_auth_mode_t auth_mode = (wifi_auth_mode_t)auth_val; // Cast int to enum

			printf("Starting hotspot '%s'...\n", ssid.c_str());
			esp_err_t err = conn_mgr.startHotspot(ssid.c_str(), pass.c_str(), channel, max_conn, hidden, auth_mode);
			if (err == ESP_OK) {
				printf("Hotspot started successfully.\n");
			} else {
				printf("Failed to start hotspot: %s\n", esp_err_to_name(err));
			}
		}
	} else if (subcmd == "off") {
		conn_mgr.stopHotspot();
		printf("Hotspot disabled.\n");
	} else {
		printf("Unknown subcommand: %s. usage: hotspot [status|on|off]\n", subcmd.c_str());
		return 1;
	}

	return 0;
}

// Command: ls - List directory contents
static int cmdLs(int argc, char** argv) {
	auto& fs = flx::services::FileSystemService::getInstance();
	auto& cli = CliService::getInstance();

	std::string path;
	if (argc > 1) {
		path = resolvePath(cli.getCurrentDirectory(), argv[1]);
	} else {
		path = cli.getCurrentDirectory();
	}

	// Simple check if path implies root but missing leading slash
	if (path.empty() || path[0] != '/') {
		// Ideally buildPath handles this, or we assume absolute if starts with /
	}

	// Listing
	auto entries = fs.listDirectory(path);
	printf("\n=== Directory: %s ===\n", path.c_str());
	if (entries.empty()) {
		printf("(empty)\n");
	} else {
		printf("%-20s %-8s %-10s\n", "Name", "Type", "Size");
		printf("----------------------------------------\n");
		for (const auto& entry: entries) {
			printf("%-20s %-8s %s\n", entry.name.c_str(), entry.isDirectory ? "DIR" : "FILE", entry.isDirectory ? "-" : flx::services::SystemInfoService::formatBytes(entry.size).c_str());
		}
	}
	printf("========================\n\n");
	return 0;
}

// Command: cd - Change directory
static int cmdCd(int argc, char** argv) {
	auto& cli = CliService::getInstance();
	std::string new_path = "/";

	if (argc > 1) {
		new_path = resolvePath(cli.getCurrentDirectory(), argv[1]);
	}

	// Verify path exists and is directory (rudimentary check using ls or just setting it)
	// For now, we just set it, effectively "trusting" the user or waiting for error on next ls
	// Practical enhancement: check if directory exists using stat/list
	cli.setCurrentDirectory(new_path);
	printf("CWD: %s\n", new_path.c_str());
	return 0;
}

// Command: pwd - Print working directory
static int cmdPwd(int /*argc*/, char** /*argv*/) {
	printf("%s\n", CliService::getInstance().getCurrentDirectory().c_str());
	return 0;
}

// Command: mkdir - Create directory
static int cmdMkdir(int argc, char** argv) {
	if (argc < 2) {
		printf("Usage: mkdir <path>\n");
		return 1;
	}
	auto& fs = flx::services::FileSystemService::getInstance();
	auto& cli = CliService::getInstance();
	std::string path = resolvePath(cli.getCurrentDirectory(), argv[1]);

	if (fs.mkdir(path)) {
		printf("Directory created: %s\n", path.c_str());
	} else {
		printf("Failed to create directory: %s\n", path.c_str());
	}
	return 0;
}

// Command: rm - Remove file or directory
static int cmdRm(int argc, char** argv) {
	if (argc < 2) {
		printf("Usage: rm <path>\n");
		return 1;
	}
	auto& fs = flx::services::FileSystemService::getInstance();
	auto& cli = CliService::getInstance();
	std::string path = resolvePath(cli.getCurrentDirectory(), argv[1]);

	// TODO: Add confirmation?
	if (fs.remove(path)) {
		printf("Removed: %s\n", path.c_str());
	} else {
		printf("Failed to remove: %s\n", path.c_str());
	}
	return 0;
}

// Command: cat - Print file contents
static int cmdCat(int argc, char** argv) {
	if (argc < 2) {
		printf("Usage: cat <file>\n");
		return 1;
	}
	auto& cli = CliService::getInstance();
	std::string path = resolvePath(cli.getCurrentDirectory(), argv[1]);

	FILE* f = fopen(path.c_str(), "r");
	if (!f) {
		printf("Failed to open file: %s\n", path.c_str());
		return 1;
	}

	printf("\n=== %s ===\n", path.c_str());
	char buffer[128];
	while (fgets(buffer, sizeof(buffer), f) != nullptr) {
		printf("%s", buffer);
	}
	printf("\n================\n\n");
	fclose(f);
	return 0;
}

// Command: brightness - Display brightness control
static int cmdBrightness(int argc, char** argv) {
	auto& display = flx::system::DisplayManager::getInstance();

	if (argc < 2) {
		printf("Brightness: %d%%\n", (int)((display.getBrightnessObservable().get() / 255.0f) * 100));
		return 0;
	}

	int val = atoi(argv[1]);
	if (val < 0) val = 0;
	if (val > 100) val = 100;

	// Scale 0-100 to 0-255
	int mapped = (val * 255) / 100;
	display.getBrightnessObservable().set(mapped);
	printf("Brightness set to %d%% (%d)\n", val, mapped);
	return 0;
}

// Command: display_test - Test low-level display driver
static int cmdDisplayTest(int argc, char** argv) {
	if (argc < 2) {
		printf("Usage: display_test <color_hex> (e.g., F800 for red)\n");
		printf("       display_test off (resumes LVGL)\n");
		return 1;
	}

	std::string arg = argv[1];
	if (arg == "off") {
		flx::core::Bundle data;
		data.putBool("paused", false);
		flx::core::EventBus::getInstance().publish("ui.gui.set_paused", data);
		printf("Resuming LVGL...\n");
		return 0;
	}

	// Parse color
	char* endptr = nullptr;
	long val = strtol(arg.c_str(), &endptr, 16);
	if (*endptr != '\0') {
		printf("Invalid color format. Use hex (e.g., F800)\n");
		return 1;
	}
	int color = (int)val;

	// Logic delegated to GuiTask
	flx::core::Bundle data;
	data.putInt32("color", color);
	flx::core::EventBus::getInstance().publish("ui.gui.run_display_test", data);
	printf("Drew test pattern with color 0x%04X.\n", color);
	printf("Touch screen to resume LVGL. CLI is still active.\n");

	return 0;
}

// Command: time - Display/Set time
static int cmdTime(int argc, char** argv) {
	if (argc > 1) {
		printf("Setting time not yet implemented via CLI (requires SNTP or manual settimeofday)\n");
		// Could implement settimeofday parsing here if needed
		return 0;
	}

	time_t now = 0;
	char strftime_buf[64];
	struct tm timeinfo = {};

	time(&now);
	localtime_r(&now, &timeinfo);
	strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);

	printf("\nCurrent Time: %s\n\n", strftime_buf);
	return 0;
}

// Command: loglevel - Set ESP log level
static int cmdLogLevel(int argc, char** argv) {
	if (argc < 3) {
		printf("Usage: loglevel <tag> <level> (none, error, warn, info, debug, verbose)\n");
		printf("       loglevel * <level> (for all tags)\n");
		return 1;
	}

	const char* tag = argv[1];
	const char* level_str = argv[2];
	esp_log_level_t level = ESP_LOG_NONE;

	if (strcmp(level_str, "none") == 0) level = ESP_LOG_NONE;
	else if (strcmp(level_str, "error") == 0)
		level = ESP_LOG_ERROR;
	else if (strcmp(level_str, "warn") == 0)
		level = ESP_LOG_WARN;
	else if (strcmp(level_str, "info") == 0)
		level = ESP_LOG_INFO;
	else if (strcmp(level_str, "debug") == 0)
		level = ESP_LOG_DEBUG;
	else if (strcmp(level_str, "verbose") == 0)
		level = ESP_LOG_VERBOSE;
	else {
		printf("Invalid log level: %s\n", level_str);
		return 1;
	}

	esp_log_level_set(tag, level);
	printf("Log level for '%s' set to %s\n", tag, level_str);
	return 0;
}

// Command: clear - Clear terminal
static int cmdClear(int /*argc*/, char** /*argv*/) {
	printf("\033[2J\033[H");
	return 0;
}

// Command: echo - Echo text
static int cmdEcho(int argc, char** argv) {
	for (int i = 1; i < argc; i++) {
		printf("%s%s", argv[i], (i < argc - 1) ? " " : "");
	}
	printf("\n");
	return 0;
}

// Command: reboot - Restart the system
static int cmdReboot(int /*argc*/, char** /*argv*/) {
	printf("\nRebooting system...\n");
	vTaskDelay(pdMS_TO_TICKS(500)); // Brief delay for message to flush
	esp_restart();
	return 0; // Never reached
}

// Command: hal - Hardware Abstraction Layer diagnostics
static int cmdHal(int argc, char** argv) {
	if (argc < 2) {
		printf("Usage: hal <command>\n");
		printf("Commands:\n");
		printf("  devices    List all registered HAL devices\n");
		printf("  health     Show HAL health report\n");
		printf("  i2c scan   Scan I2C bus for devices\n");
		return 1;
	}

	std::string subcmd = argv[1];

	if (subcmd == "devices") {
		auto& registry = flx::hal::DeviceRegistry::getInstance();
		auto devices = registry.getAll();
		printf("\n=== HAL Devices ===\n");
		printf("%-4s %-12s %-20s %-10s\n", "ID", "Type", "Name", "State");
		printf("--------------------------------------------------\n");
		for (const auto& dev: devices) {
			printf("%-4lu %-12s %-20.*s %-10d\n", dev->getId(), flx::hal::IDevice::typeToString(dev->getType()), static_cast<int>(dev->getName().length()), dev->getName().data(), static_cast<int>(dev->getState()));
		}
		printf("===================\n\n");
	} else if (subcmd == "health") {
		auto& registry = flx::hal::DeviceRegistry::getInstance();
		auto report = registry.getHealthReport();
		printf("\n=== HAL Health Report ===\n");
		printf("Total Devices:   %zu\n", report.totalDevices);
		printf("Healthy Devices: %zu\n", report.healthyDevices);
		printf("Error Devices:   %zu\n", report.errorDevices);
		if (!report.unhealthyDevices.empty()) {
			printf("Unhealthy Device IDs: ");
			for (const auto& d: report.unhealthyDevices) {
				printf("%lu ", d.first);
			}
			printf("\n");
		}
		printf("=========================\n\n");
	} else if (subcmd == "i2c" && argc > 2 && strcmp(argv[2], "scan") == 0) {
		auto& registry = flx::hal::DeviceRegistry::getInstance();
		auto i2cBus = registry.findFirst<flx::hal::i2c::II2cBus>(flx::hal::IDevice::Type::I2c);
		if (!i2cBus) {
			printf("No I2C bus found in DeviceRegistry.\n");
			return 1;
		}
		printf("Scanning I2C bus %d...\n", i2cBus->getPort());
		auto devices = i2cBus->scan(10);
		if (devices.empty()) {
			printf("No I2C devices found.\n");
		} else {
			printf("Found devices at addresses: ");
			for (uint8_t addr: devices) {
				printf("0x%02X ", addr);
			}
			printf("\n");
		}
	} else {
		printf("Unknown HAL command: %s\n", subcmd.c_str());
		return 1;
	}
	return 0;
}

#define REGISTER_CLI_CMD(name, help_text, handler)       \
	{                                                    \
		const esp_console_cmd_t cmd = {                  \
			.command = (name),                           \
			.help = (help_text),                         \
			.hint = nullptr,                             \
			.func = (handler),                           \
			.argtable = nullptr,                         \
			.func_w_context = nullptr,                   \
			.context = nullptr                           \
		};                                               \
		ESP_ERROR_CHECK(esp_console_cmd_register(&cmd)); \
	}

void CliService::registerCommands() {
	REGISTER_CLI_CMD("sysinfo", "Display generic system information", &cmdSysInfo);
	REGISTER_CLI_CMD("heap", "Display heap memory statistics", &cmdHeap);
	REGISTER_CLI_CMD("uptime", "Display system uptime", &cmdUptime);
	REGISTER_CLI_CMD("reboot", "Restart the system", &cmdReboot);

	// Phase 1: New System Info Commands
	REGISTER_CLI_CMD("tasks", "List FreeRTOS tasks and stats", &cmdTasks);
	REGISTER_CLI_CMD("storage", "Display partition/storage usage", &cmdStorage);
	REGISTER_CLI_CMD("psram", "Display PSRAM statistics", &cmdPsram);
	REGISTER_CLI_CMD("version", "Show FlxOS software versions", &cmdVersion);
	REGISTER_CLI_CMD("chip", "Show hardware/chip details", &cmdChip);

	// Phase 2: Networking Commands
	REGISTER_CLI_CMD("wifi", "WiFi management (status, scan, connect <ssid> <pass>, on, off)", &cmdWiFi);
	REGISTER_CLI_CMD("hotspot", "Hotspot management (status, on, off)", &cmdHotspot);

	// Phase 3: Filesystem Commands
	REGISTER_CLI_CMD("ls", "List directory contents", &cmdLs);
	REGISTER_CLI_CMD("cd", "Change working directory", &cmdCd);
	REGISTER_CLI_CMD("pwd", "Print working directory", &cmdPwd);
	REGISTER_CLI_CMD("mkdir", "Create directory", &cmdMkdir);
	REGISTER_CLI_CMD("rm", "Remove file or directory", &cmdRm);
	REGISTER_CLI_CMD("cat", "Print file contents", &cmdCat);
	REGISTER_CLI_CMD("df", "Display filesystem usage", &cmdStorage); // Alias

	// Phase 4: System Control
	REGISTER_CLI_CMD("brightness", "Set display brightness (0-100)", &cmdBrightness);
	REGISTER_CLI_CMD("display_test", "Test low-level display driver (color_hex or off)", &cmdDisplayTest);
	REGISTER_CLI_CMD("time", "Show system time", &cmdTime);
	REGISTER_CLI_CMD("loglevel", "Set log level for tags", &cmdLogLevel);
	REGISTER_CLI_CMD("clear", "Clear terminal screen", &cmdClear);
	REGISTER_CLI_CMD("echo", "Echo text to stdout", &cmdEcho);
	REGISTER_CLI_CMD("free", "Show memory stats", &cmdHeap); // Alias
	REGISTER_CLI_CMD("top", "Show task list", &cmdTasks); // Alias
	REGISTER_CLI_CMD("hal", "HAL diagnostics (devices, health, i2c scan)", &cmdHal);

	Log::info(TAG, "Registered CLI commands: sysinfo, heap, uptime, reboot, tasks, storage, psram, version, chip, wifi, hotspot, ls, cd, pwd, mkdir, rm, cat, df, brightness, time, loglevel, clear, echo, free, top");
}

bool CliService::onStart() {
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
#if defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
	esp_console_dev_usb_serial_jtag_config_t const hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl));
#else
	esp_console_dev_uart_config_t const hw_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_console_new_repl_uart(&hw_config, &repl_config, &repl));
#endif

	// Start REPL
	ESP_ERROR_CHECK(esp_console_start_repl(repl));

	Log::info(TAG, "CLI started. Type 'help' for available commands.");
	return true;
}

void CliService::onStop() {
	Log::info(TAG, "CLI service stopped");
}

} // namespace flx::system
