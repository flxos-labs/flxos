#include "SystemInfoService.hpp"
#include "core/common/Logger.hpp"
#include "core/connectivity/ConnectivityManager.hpp"
#include "display/lv_display.h"
#include "esp_chip_info.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_idf_version.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_vfs_fat.h"
#include "esp_wifi.h"
#include "esp_wifi_types_generic.h"
#include "freertos/task.h"
#if defined(CONFIG_FLXOS_BATTERY_ENABLED)
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#endif
#include "misc/lv_color.h"
#include "misc/lv_types.h"
#include "portmacro.h"
#include "sdkconfig.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string_view>

static constexpr std::string_view TAG = "SystemInfo";

static constexpr const char* FLXOS_VERSION = "0.1.0-alpha";

namespace System::Services {

SystemInfoService& SystemInfoService::getInstance() {
	static SystemInfoService instance;
	static bool initialized = false;
	if (!initialized) {
		Log::info(TAG, "SystemInfoService initialized");
#if defined(CONFIG_FLXOS_BATTERY_ENABLED)
		instance.initBatteryAdc();
#endif
		initialized = true;
	}
	return instance;
}

#if defined(CONFIG_FLXOS_BATTERY_ENABLED)
void SystemInfoService::initBatteryAdc() {
	if (m_batteryInitialized)
		return;

	Log::info(TAG, "Initializing Battery ADC...");

	adc_oneshot_unit_handle_t handle;
	adc_oneshot_unit_init_cfg_t init_config = {
		.unit_id = (adc_unit_t)(CONFIG_FLXOS_BATTERY_ADC_UNIT_1 ? ADC_UNIT_1 : ADC_UNIT_2),
		.clk_src = ADC_DIGI_CLK_SRC_DEFAULT,
		.ulp_mode = ADC_ULP_MODE_DISABLE,
	};
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &handle));
	m_adcHandle = handle;

	adc_oneshot_chan_cfg_t config = {
		.atten = ADC_ATTEN_DB_12,
		.bitwidth = ADC_BITWIDTH_DEFAULT,
	};
	ESP_ERROR_CHECK(adc_oneshot_config_channel(handle, (adc_channel_t)CONFIG_FLXOS_BATTERY_ADC_CHANNEL, &config));

	// Calibration
	adc_cali_handle_t cali_handle = nullptr;
	adc_cali_curve_fitting_config_t cali_config = {
		.unit_id = (adc_unit_t)(CONFIG_FLXOS_BATTERY_ADC_UNIT_1 ? ADC_UNIT_1 : ADC_UNIT_2),
		.chan = (adc_channel_t)CONFIG_FLXOS_BATTERY_ADC_CHANNEL,
		.atten = ADC_ATTEN_DB_12,
		.bitwidth = ADC_BITWIDTH_DEFAULT,
	};

	esp_err_t const ret = adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle);
	if (ret == ESP_OK) {
		m_adcCaliHandle = cali_handle;
		Log::info(TAG, "ADC Calibration scheme created");
	} else {
		Log::warn(TAG, "ADC Calibration failed or not available");
	}

	m_batteryInitialized = true;
}
#endif

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
	stats.bootReason = getResetReason();
	stats.buildDate = __DATE__ " " __TIME__;

	esp_chip_info_t chip_info;
	esp_chip_info(&chip_info);
	stats.cores = chip_info.cores;
	stats.revision = chip_info.revision;
	stats.cpuFreqMhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;

	// Build features string (avoid stringstream allocation)
	stats.features.clear();
	stats.features.reserve(16);
	if (chip_info.features & CHIP_FEATURE_WIFI_BGN) {
		stats.features += "WiFi ";
	}
	if (chip_info.features & CHIP_FEATURE_BT) {
		stats.features += "BT ";
	}
	if (chip_info.features & CHIP_FEATURE_BLE) {
		stats.features += "BLE";
	}

	// Get uptime
	int64_t const uptime_us = esp_timer_get_time();
	stats.uptimeSeconds = uptime_us / 1000000;

	// Display Info
	stats.displayResX = 0;
	stats.displayResY = 0;
	stats.displayBpp = 0;
	stats.colorFormat = "Unknown";
#if !CONFIG_FLXOS_HEADLESS_MODE
	lv_display_t* disp = lv_display_get_default();
	if (disp) {
		stats.displayResX = lv_display_get_horizontal_resolution(disp);
		stats.displayResY = lv_display_get_vertical_resolution(disp);
		lv_color_format_t const cf = lv_display_get_color_format(disp);

		switch (cf) {
			case LV_COLOR_FORMAT_RGB565:
				stats.colorFormat = "RGB565";
				stats.displayBpp = 16;
				break;
			case LV_COLOR_FORMAT_RGB565A8:
				stats.colorFormat = "RGB565A8";
				stats.displayBpp = 24;
				break;
			case LV_COLOR_FORMAT_RGB888:
				stats.colorFormat = "RGB888";
				stats.displayBpp = 24;
				break;
			case LV_COLOR_FORMAT_XRGB8888:
				stats.colorFormat = "XRGB8888";
				stats.displayBpp = 32;
				break;
			case LV_COLOR_FORMAT_ARGB8888:
				stats.colorFormat = "ARGB8888";
				stats.displayBpp = 32;
				break;

			// Swapped formats
			case LV_COLOR_FORMAT_RGB565_SWAPPED:
				stats.colorFormat = "RGB565 (Swapped)";
				stats.displayBpp = 16;
				break;

			// Others
			case LV_COLOR_FORMAT_L8:
				stats.colorFormat = "L8 (Grayscale)";
				stats.displayBpp = 8;
				break;
			case LV_COLOR_FORMAT_A8:
				stats.colorFormat = "A8 (Alpha)";
				stats.displayBpp = 8;
				break;
			case LV_COLOR_FORMAT_I8:
				stats.colorFormat = "I8 (Indexed)";
				stats.displayBpp = 8;
				break;

			default:
				stats.colorFormat = "Format " + std::to_string((int)cf);
				break;
		}
	}
#endif

	return stats;
}

MemoryStats SystemInfoService::getMemoryStats() {
	MemoryStats stats {};

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
		uint64_t total = 0;
		uint64_t free = 0;
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
	BatteryStats stats {};
#if defined(CONFIG_FLXOS_BATTERY_ENABLED)
	stats.isConfigured = true;
	if (m_batteryInitialized && m_adcHandle) {
		int raw = 0;
		if (adc_oneshot_read((adc_oneshot_unit_handle_t)m_adcHandle, (adc_channel_t)CONFIG_FLXOS_BATTERY_ADC_CHANNEL, &raw) == ESP_OK) {
			int voltage = 0;
			if (m_adcCaliHandle) {
				adc_cali_raw_to_voltage((adc_cali_handle_t)m_adcCaliHandle, raw, &voltage);
			} else {
				// Fallback to simple calculation if calibration is missing
				voltage = (raw * 3300) / 4095;
			}

			// Apply divider factor
			voltage = (int)(voltage * (CONFIG_FLXOS_BATTERY_DIVIDER_FACTOR / 100.0));

			// Map to percentage
			int const min_v = CONFIG_FLXOS_BATTERY_VOLTAGE_MIN;
			int const max_v = CONFIG_FLXOS_BATTERY_VOLTAGE_MAX;

			if (voltage >= max_v) {
				stats.level = 100;
			} else if (voltage <= min_v) {
				stats.level = 0;
			} else {
				stats.level = (voltage - min_v) * 100 / (max_v - min_v);
			}

			// For now, charging status is hardcoded as false unless we have a specific PMIC/GPIO for it
			stats.isCharging = false;
			return stats;
		}
	}
#else
	stats.isConfigured = false;
#endif
	// Fallback
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
			if (stats.rssi > -50) {
				stats.signalStrength = "Excellent";
			} else if (stats.rssi > -60) {
				stats.signalStrength = "Good";
			} else if (stats.rssi > -70) {
				stats.signalStrength = "Fair";
			} else {
				stats.signalStrength = "Weak";
			}
		}

		// Retrieve IP address from ConnectivityManager
		const char* ip = ConnectivityManager::getInstance().getWiFiIpObservable().get();
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

	// Use stack allocation for small task counts, heap for larger
	constexpr size_t STACK_TASK_LIMIT = 32;
	TaskStatus_t stack_array[STACK_TASK_LIMIT];
	TaskStatus_t* task_array = (task_count <= STACK_TASK_LIMIT) ? stack_array : new TaskStatus_t[task_count];

	uint32_t total_runtime = 0;
	task_count = uxTaskGetSystemState(task_array, task_count, &total_runtime);

	// Limit to maxTasks if specified
	size_t const count = (maxTasks == 0 || task_count < maxTasks) ? task_count : maxTasks;

	uint32_t const runtime_delta = total_runtime - m_lastTotalRuntime;
	m_lastTotalRuntime = total_runtime;

	// Cache core count (static, never changes)
	static int const cores = []() {
		esp_chip_info_t chip_info;
		esp_chip_info(&chip_info);
		return chip_info.cores;
	}();

	for (size_t i = 0; i < count; i++) {
		TaskInfo info;
		info.name = task_array[i].pcTaskName;
		info.stackHighWaterMark = task_array[i].usStackHighWaterMark;
		info.currentPriority = task_array[i].uxCurrentPriority;
		info.basePriority = task_array[i].uxBasePriority;
		info.coreID = (int)task_array[i].xCoreID;
		info.runtime = task_array[i].ulRunTimeCounter;

		// Calculate CPU usage
		if (runtime_delta > 0) {
			TaskHandle_t handle = task_array[i].xHandle;
			auto it = m_taskTracking.find(handle);
			if (it != m_taskTracking.end()) {
				uint32_t task_runtime_delta = info.runtime - it->second.lastRuntime;

				// Safeguard against counter wrap or task restart (deletion/creation with same name)
				if (task_runtime_delta > runtime_delta) {
					task_runtime_delta = 0;
				}

				// Calculate Usage: (TaskDelta / TotalDelta) * 100 * Cores
				// This normalizes 100% to be one fully loaded core
				info.cpuUsagePercent = ((float)task_runtime_delta * 100.0F / (float)runtime_delta) * cores;
			} else {
				info.cpuUsagePercent = 0.0F;
			}
			m_taskTracking[handle] = {info.runtime, (uint32_t)esp_timer_get_time()};
		} else {
			info.cpuUsagePercent = 0.0F;
		}

		switch (task_array[i].eCurrentState) {
			case eRunning:
				info.state = "RUN";
				break;
			case eReady:
				info.state = "RDY";
				break;
			case eBlocked:
				info.state = "BLK";
				break;
			case eSuspended:
				info.state = "SUS";
				break;
			case eDeleted:
				info.state = "DEL";
				break;
			default:
				info.state = "INV";
				break;
		}

		tasks.push_back(info);
	}

	// Only delete if we used heap allocation
	if (task_count > STACK_TASK_LIMIT) {
		delete[] task_array;
	}

	return tasks;
}

std::string SystemInfoService::formatBytes(uint32_t bytes) {
	char buffer[32];
	if (bytes < 1024) {
		snprintf(buffer, sizeof(buffer), "%lu B", (unsigned long)bytes);
	} else if (bytes < 1024 * 1024) {
		snprintf(buffer, sizeof(buffer), "%.2f KB", bytes / 1024.0);
	} else {
		snprintf(buffer, sizeof(buffer), "%.2f MB", bytes / (1024.0 * 1024.0));
	}
	return std::string(buffer);
}

std::string SystemInfoService::getResetReason() {
	esp_reset_reason_t const reason = esp_reset_reason();
	switch (reason) {
		case ESP_RST_POWERON:
			return "Power On";
		case ESP_RST_EXT:
			return "External Pin";
		case ESP_RST_SW:
			return "Software Reset";
		case ESP_RST_PANIC:
			return "Exception/Panic";
		case ESP_RST_INT_WDT:
			return "Interrupt WDT";
		case ESP_RST_TASK_WDT:
			return "Task WDT";
		case ESP_RST_WDT:
			return "Other WDT";
		case ESP_RST_DEEPSLEEP:
			return "Deep Sleep Wakeup";
		case ESP_RST_BROWNOUT:
			return "Brownout";
		case ESP_RST_SDIO:
			return "SDIO Reset";
		default:
			return "Unknown";
	}
}

} // namespace System::Services
