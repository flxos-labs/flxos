#include "SystemInfoService.hpp"
#include "core/connectivity/ConnectivityManager.hpp"
#include "esp_chip_info.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <iomanip>
#include <sstream>

static const char* TAG = "SystemInfoService";
static constexpr const char* FLXOS_VERSION = "0.1.0-alpha";

namespace System {
namespace Services {

SystemInfoService& SystemInfoService::getInstance() {
	static SystemInfoService instance;
	return instance;
}

std::string SystemInfoService::getChipModel() {
#ifdef CONFIG_IDF_TARGET_ESP32
	return "ESP32";
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
	return "ESP32-S2";
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
	return "ESP32-S3";
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
	return "ESP32-C3";
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
	return "ESP32-C6";
#elif defined(CONFIG_IDF_TARGET_ESP32H2)
	return "ESP32-H2";
#else
	return "Unknown";
#endif
}

SystemStats SystemInfoService::getSystemStats() {
	SystemStats stats;

	stats.flxosVersion = FLXOS_VERSION;
	stats.idfVersion = esp_get_idf_version();
	stats.chipModel = getChipModel();

	esp_chip_info_t chip_info;
	esp_chip_info(&chip_info);
	stats.cores = chip_info.cores;
	stats.revision = chip_info.revision;

	// Build features string
	std::stringstream features;
	if (chip_info.features & CHIP_FEATURE_WIFI_BGN)
		features << "WiFi ";
	if (chip_info.features & CHIP_FEATURE_BT)
		features << "BT ";
	if (chip_info.features & CHIP_FEATURE_BLE)
		features << "BLE";
	stats.features = features.str();

	// Get uptime
	int64_t uptime_us = esp_timer_get_time();
	stats.uptimeSeconds = uptime_us / 1000000;

	ESP_LOGD(TAG, "System stats retrieved: %s, %d cores, uptime: %llu s", stats.chipModel.c_str(), stats.cores, stats.uptimeSeconds);

	return stats;
}

MemoryStats SystemInfoService::getMemoryStats() {
	MemoryStats stats;

	stats.freeHeap = esp_get_free_heap_size();
	stats.minFreeHeap = esp_get_minimum_free_heap_size();
	stats.totalHeap = heap_caps_get_total_size(MALLOC_CAP_8BIT);
	stats.usedHeap = stats.totalHeap - stats.freeHeap;
	stats.usagePercent = (stats.usedHeap * 100) / stats.totalHeap;

	// PSRAM info
	stats.totalPsram = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
	stats.freePsram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
	stats.hasPsram = (stats.totalPsram > 0);

	ESP_LOGD(TAG, "Memory stats: Total=%u, Free=%u, Used=%u (%d%%)", stats.totalHeap, stats.freeHeap, stats.usedHeap, stats.usagePercent);

	return stats;
}

WiFiStats SystemInfoService::getWiFiStats() {
	WiFiStats stats;

	stats.connected = ConnectivityManager::getInstance().isWiFiConnected();

	// Initialize defaults
	stats.ssid = "";
	stats.ipAddress = "";
	stats.rssi = 0;
	stats.signalStrength = "Unknown";

	// Get MAC address
	esp_read_mac(stats.mac, ESP_MAC_WIFI_STA);

	if (stats.connected) {
		wifi_ap_record_t ap_info;
		if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
			stats.ssid = std::string(reinterpret_cast<const char*>(ap_info.ssid));
			stats.rssi = ap_info.rssi;

			// Determine signal strength
			if (stats.rssi > -50)
				stats.signalStrength = "Excellent";
			else if (stats.rssi > -60)
				stats.signalStrength = "Good";
			else if (stats.rssi > -70)
				stats.signalStrength = "Fair";
			else
				stats.signalStrength = "Weak";

			ESP_LOGD(TAG, "WiFi stats: SSID=%s, RSSI=%d (%s)", stats.ssid.c_str(), stats.rssi, stats.signalStrength.c_str());
		}

		// IP address would come from connectivity manager subject
		// For now, just indicate it's via DHCP
		stats.ipAddress = "(via DHCP)";
	}

	return stats;
}

std::vector<TaskInfo> SystemInfoService::getTaskList(size_t maxTasks) {
	std::vector<TaskInfo> tasks;

	UBaseType_t task_count = uxTaskGetNumberOfTasks();
	if (task_count == 0) {
		ESP_LOGW(TAG, "No tasks found");
		return tasks;
	}

	TaskStatus_t* task_array = new TaskStatus_t[task_count];
	uint32_t total_runtime;
	task_count = uxTaskGetSystemState(task_array, task_count, &total_runtime);

	// Limit to maxTasks if specified
	size_t count = (maxTasks == 0 || task_count < maxTasks) ? task_count : maxTasks;

	for (size_t i = 0; i < count; i++) {
		TaskInfo info;
		info.name = task_array[i].pcTaskName;
		info.stackHighWaterMark = task_array[i].usStackHighWaterMark;
		tasks.push_back(info);
	}

	delete[] task_array;

	ESP_LOGD(TAG, "Retrieved %d tasks (out of %d total)", count, task_count);
	return tasks;
}

std::string SystemInfoService::formatBytes(uint32_t bytes) {
	std::stringstream ss;
	if (bytes < 1024) {
		ss << bytes << " B";
	} else if (bytes < 1024 * 1024) {
		ss << std::fixed << std::setprecision(2) << (bytes / 1024.0) << " KB";
	} else {
		ss << std::fixed << std::setprecision(2) << (bytes / (1024.0 * 1024.0)) << " MB";
	}
	return ss.str();
}

} // namespace Services
} // namespace System
