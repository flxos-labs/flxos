#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace System {
namespace Services {

struct SystemStats {
	std::string flxosVersion;
	std::string idfVersion;
	std::string chipModel;
	int cores;
	int revision;
	std::string features;
	uint64_t uptimeSeconds;
	uint32_t cpuFreqMhz;
};

struct MemoryStats {
	uint32_t totalHeap;
	uint32_t freeHeap;
	uint32_t minFreeHeap;
	uint32_t usedHeap;
	int usagePercent;
	uint32_t largestFreeBlock;
	uint32_t totalPsram;
	uint32_t freePsram;
	uint32_t usedPsram;
	int usagePercentPsram;
	bool hasPsram;
};

struct StorageStats {
	std::string name;
	size_t totalBytes;
	size_t usedBytes;
	size_t freeBytes;
};

struct BatteryStats {
	int level; // 0-100
	bool isCharging;
};

struct WiFiStats {
	bool connected;
	std::string ssid;
	std::string ipAddress;
	int8_t rssi;
	std::string signalStrength; // "Excellent", "Good", "Fair", "Weak"
	uint8_t mac[6];
};

struct TaskInfo {
	std::string name;
	uint32_t stackHighWaterMark;
	std::string state; // "Ready", "Running", "Blocked", "Suspended", "Deleted"
	int currentPriority;
	int basePriority;
	int coreID;
	uint32_t runtime; // Runtime counter
};

class SystemInfoService {
public:

	static SystemInfoService& getInstance();

	/**
	 * Get system information (version, chip, uptime, cpu freq)
	 */
	SystemStats getSystemStats();

	/**
	 * Get memory statistics (heap, PSRAM, largest free block)
	 */
	MemoryStats getMemoryStats();

	/**
	 * Get storage statistics for partitions
	 */
	std::vector<StorageStats> getStorageStats();

	/**
	 * Get battery statistics (placeholder)
	 */
	BatteryStats getBatteryStats();

	/**
	 * Get WiFi connection statistics
	 */
	WiFiStats getWiFiStats();

	/**
	 * Get list of running FreeRTOS tasks
	 * @param maxTasks Maximum number of tasks to return (0 for all)
	 */
	std::vector<TaskInfo> getTaskList(size_t maxTasks = 0);

	/**
	 * Format bytes to human-readable string (B, KB, MB)
	 */
	static std::string formatBytes(uint32_t bytes);

private:

	SystemInfoService() = default;
	~SystemInfoService() = default;
	SystemInfoService(const SystemInfoService&) = delete;
	SystemInfoService& operator=(const SystemInfoService&) = delete;

	std::string getChipModel();
};

} // namespace Services
} // namespace System
