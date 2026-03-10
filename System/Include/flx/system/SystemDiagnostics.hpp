#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <flx/apps/AppManager.hpp>
#include <flx/services/IService.hpp>
#include <flx/services/ServiceManifest.hpp>

namespace flx::system {

/**
 * @brief Memory statistics for system diagnostics.
 */
struct MemoryStats {
	uint32_t freeHeapBytes = 0;        ///< Current free heap in bytes
	uint32_t totalHeapBytes = 0;       ///< Total heap capacity
	uint32_t minFreeHeapBytes = 0;     ///< Minimum free heap since boot (watermark)
	uint32_t largestFreeBlock = 0;     ///< Largest contiguous free block
};

/**
 * @brief Unified snapshot of the entire system's health.
 *
 * Aggregates service states, app states, memory stats, and boot timing
 * into a single queryable structure.  The SystemInfo app or any remote
 * monitoring endpoint can call getSystemDiagnostics() once to obtain the
 * complete picture.
 */
struct SystemDiagnostics {

	// ─── Services ───

	struct ServiceInfo {
		std::string id;
		std::string name;
		std::string version;
		flx::services::ServiceState state = flx::services::ServiceState::Stopped;
		flx::services::ServiceStats stats {};
		flx::services::HealthStatus lastHealthStatus = flx::services::HealthStatus::Healthy;
		bool required = false;
		bool autoRestart = false;
		std::vector<std::string> groups {};
	};
	std::vector<ServiceInfo> services;

	// ─── Apps ───

	struct AppInfo {
		std::string packageName;
		std::string appName;
		bool isActive = false;     ///< Currently in the app stack
		bool isForeground = false; ///< Top of the app stack
		flx::apps::AppLaunchStats stats {};
		bool isBlocked = false;    ///< Blocked due to crash recovery
		uint32_t crashCount = 0;
	};
	std::vector<AppInfo> apps;

	// ─── System ───

	MemoryStats memory {};
	int64_t uptimeUs = 0;         ///< System uptime in microseconds
	int64_t bootTimeUs = 0;       ///< Total boot timeline span in microseconds
	size_t bootTimelineEntries = 0; ///< Number of recorded boot timeline events
	size_t taskCount = 0;         ///< Number of FreeRTOS tasks

	// ─── Summary counts ───

	uint32_t servicesRegistered = 0;
	uint32_t servicesRunning = 0;
	uint32_t servicesFailed = 0;
	uint32_t appsRegistered = 0;
	uint32_t appsInStack = 0;
};

/**
 * @brief Collect a complete system diagnostics snapshot.
 *
 * Queries ServiceRegistry, AppManager, BootTimeline, and ESP-IDF system
 * APIs to produce a single unified view of the system's state.
 *
 * Thread-safe: acquires appropriate locks internally.
 */
SystemDiagnostics getSystemDiagnostics();

/**
 * @brief Log a human-readable summary of system diagnostics.
 */
void dumpSystemDiagnostics();

} // namespace flx::system
