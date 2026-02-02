#include "SystemInfoService.hpp"
#include "core/common/Logger.hpp"
#include "core/connectivity/ConnectivityManager.hpp"
#include "esp_chip_info.h"
#include "esp_heap_caps.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_vfs_fat.h"
#include "esp_wifi.h"
#include "freertos/task.h"
#include <iomanip>
#include <sstream>
#include <string_view>

static constexpr std::string_view TAG = "SystemInfo";

static constexpr const char* FLXOS_VERSION = "0.1.0-alpha";

namespace System {
namespace Services {

SystemInfoService& SystemInfoService::getInstance() {
	static SystemInfoService instance;
	static bool initialized = false;
	if (!initialized) {
		Log::info(TAG, "SystemInfoService initialized");
		initialized = true;
	}
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
	stats.cpuFreqMhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;

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

	return stats;
}

MemoryStats SystemInfoService::getMemoryStats() {
	MemoryStats stats;

	stats.freeHeap = esp_get_free_heap_size();
	stats.minFreeHeap = esp_get_minimum_free_heap_size();
	stats.totalHeap = heap_caps_get_total_size(MALLOC_CAP_8BIT);
	stats.usedHeap = stats.totalHeap - stats.freeHeap;
	stats.usagePercent = (stats.usedHeap * 100) / stats.totalHeap;
	stats.largestFreeBlock = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);

	// PSRAM info
	stats.totalPsram = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
	stats.freePsram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
	stats.hasPsram = (stats.totalPsram > 0);
	if (stats.hasPsram) {
		stats.usedPsram = stats.totalPsram - stats.freePsram;
		stats.usagePercentPsram = (stats.usedPsram * 100) / stats.totalPsram;
	} else {
		stats.usedPsram = 0;
		stats.usagePercentPsram = 0;
	}

	return stats;
}

std::vector<StorageStats> SystemInfoService::getStorageStats() {
	std::vector<StorageStats> storageStats;

	// Helper lambda to get stats for a path
	auto addStats = [&](const std::string& name, const char* path) {
		uint64_t total = 0, free = 0;
		if (esp_vfs_fat_info(path, &total, &free) == ESP_OK) {
			StorageStats stats;
			stats.name = name;
			stats.totalBytes = (size_t)total;
			stats.freeBytes = (size_t)free;
			stats.usedBytes = stats.totalBytes - stats.freeBytes;
			storageStats.push_back(stats);
		}
	};

	addStats("System", "/system");
	addStats("Data", "/data");

	return storageStats;
}

BatteryStats SystemInfoService::getBatteryStats() {
	BatteryStats stats;
	// Placeholder: Assume full battery and not charging for now
	stats.level = 100;
	stats.isCharging = false;
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
		}

		// Retrieve IP address from ConnectivityManager
		lv_subject_t& ipSubject = ConnectivityManager::getInstance().getWiFiIpSubject();
		const char* ip = lv_subject_get_string(&ipSubject);
		stats.ipAddress = ip ? std::string(ip) : "0.0.0.0";
	}

	return stats;
}

std::vector<TaskInfo> SystemInfoService::getTaskList(size_t maxTasks) {
	std::vector<TaskInfo> tasks;

	UBaseType_t task_count = uxTaskGetNumberOfTasks();
	if (task_count == 0) {
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
		info.currentPriority = task_array[i].uxCurrentPriority;
		info.basePriority = task_array[i].uxBasePriority;
		info.coreID = (int)task_array[i].xCoreID;
		info.runtime = task_array[i].ulRunTimeCounter;

		switch (task_array[i].eCurrentState) {
			case eRunning:
				info.state = "Running";
				break;
			case eReady:
				info.state = "Ready";
				break;
			case eBlocked:
				info.state = "Blocked";
				break;
			case eSuspended:
				info.state = "Suspend";
				break;
			case eDeleted:
				info.state = "Deleted";
				break;
			default:
				info.state = "Invalid";
				break;
		}

		tasks.push_back(info);
	}

	delete[] task_array;

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
