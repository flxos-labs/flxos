#pragma once

#include <cstdint>
#include <flx/apps/AppManager.hpp>
#include <flx/services/IService.hpp>
#include <flx/services/ServiceManifest.hpp>
#include <string>
#include <vector>

namespace flx::system {

/**
 * @brief Unified system diagnostics snapshot.
 *
 * Aggregates health information from services, apps, and system resources
 * into a single queryable structure. Makes building a SystemInfo app trivial
 * and enables remote monitoring.
 */
struct SystemDiagnostics {

	// ── Services ──

	struct ServiceInfo {
		std::string id;
		std::string name;
		flx::services::ServiceState state;
		flx::services::ServiceStats stats;
		flx::services::HealthStatus lastHealthStatus;
		bool required;
		std::string version;
	};
	std::vector<ServiceInfo> services;

	// ── Apps ──

	struct AppInfo {
		std::string packageName;
		std::string appName;
		bool isActive;
		flx::apps::AppLaunchStats stats;
		bool isBlocked;
		uint32_t crashCount;
	};
	std::vector<AppInfo> apps;

	// ── Memory ──

	struct MemoryStats {
		uint32_t freeHeapBytes;
		uint32_t minFreeHeapBytes;
		uint32_t totalHeapBytes;
	};
	MemoryStats memory {};

	// ── System ──

	int64_t uptimeUs; ///< System uptime in microseconds
	int64_t bootTimeUs; ///< Total boot time from BootTimeline
	size_t taskCount; ///< Number of FreeRTOS tasks

	// ── Counts ──

	size_t serviceCount;
	size_t appCount;
	size_t activeAppCount;
};

/**
 * Collect a full system diagnostics snapshot.
 * Queries ServiceRegistry, AppManager, and system APIs.
 */
SystemDiagnostics getSystemDiagnostics();

/**
 * Dump system diagnostics to the log.
 */
void dumpSystemDiagnostics();

} // namespace flx::system
