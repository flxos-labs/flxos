#include <flx/system/SystemDiagnostics.hpp>

#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <flx/apps/AppManager.hpp>
#include <flx/core/BootTimeline.hpp>
#include <flx/core/Logger.hpp>
#include <flx/services/ServiceRegistry.hpp>

#include <cinttypes>

static constexpr const char* TAG = "SystemDiagnostics";

namespace flx::system {

SystemDiagnostics getSystemDiagnostics() {
	SystemDiagnostics diag;

	// ── Services ──
	auto& registry = flx::services::ServiceRegistry::getInstance();
	const auto& allServices = registry.getAllServices();
	diag.serviceCount = allServices.size();

	for (const auto& svc : allServices) {
		if (!svc) continue;
		const auto& manifest = svc->getManifest();
		auto stats = svc->getServiceStats();

		SystemDiagnostics::ServiceInfo info;
		info.id = manifest.serviceId;
		info.name = manifest.serviceName;
		info.state = svc->getState();
		info.stats = stats;
		info.lastHealthStatus = svc->onHealthCheck();
		info.required = manifest.required;
		info.version = manifest.version;
		diag.services.push_back(std::move(info));
	}

	// ── Apps ──
	auto& appMgr = flx::apps::AppManager::getInstance();
	const auto& installedApps = appMgr.getInstalledApps();
	diag.appCount = installedApps.size();
	diag.activeAppCount = 0;

	for (const auto& app : installedApps) {
		if (!app) continue;

		SystemDiagnostics::AppInfo info;
		info.packageName = app->getPackageName();
		info.appName = app->getAppName();
		info.isActive = app->isActive();
		info.stats = appMgr.getAppStats(info.packageName);
		info.isBlocked = appMgr.isAppBlocked(info.packageName);
		info.crashCount = 0; // crash count is internal to AppCrashRecord
		diag.apps.push_back(std::move(info));

		if (info.isActive) diag.activeAppCount++;
	}

	// ── Memory ──
	diag.memory.freeHeapBytes = esp_get_free_heap_size();
	diag.memory.minFreeHeapBytes = esp_get_minimum_free_heap_size();
	diag.memory.totalHeapBytes = diag.memory.freeHeapBytes; // ESP-IDF doesn't have a direct total getter

	// ── System ──
	diag.uptimeUs = esp_timer_get_time();
	diag.bootTimeUs = flx::core::BootTimeline::getInstance().getTotalBootTimeUs();
	diag.taskCount = uxTaskGetNumberOfTasks();

	return diag;
}

void dumpSystemDiagnostics() {
	auto diag = getSystemDiagnostics();

	Log::info(TAG, "╔══════════════════════════════════════════════╗");
	Log::info(TAG, "║         SYSTEM DIAGNOSTICS SNAPSHOT          ║");
	Log::info(TAG, "╚══════════════════════════════════════════════╝");

	// System overview
	int64_t uptimeSec = diag.uptimeUs / 1000000;
	Log::info(TAG, "Uptime: %" PRId64 "s | Boot: %" PRId64 " ms | Tasks: %zu",
	          uptimeSec, diag.bootTimeUs / 1000, diag.taskCount);

	// Memory
	Log::info(TAG, "Memory: free=%" PRIu32 " KB, min_free=%" PRIu32 " KB",
	          diag.memory.freeHeapBytes / 1024,
	          diag.memory.minFreeHeapBytes / 1024);

	// Services
	Log::info(TAG, "─── Services (%zu) ───", diag.serviceCount);
	for (const auto& svc : diag.services) {
		const char* stateStr = flx::services::serviceStateToString(svc.state);
		Log::info(TAG, "  [%s] %s (%s) v%s — starts: %" PRIu32 ", boot: %" PRId64 " ms, heap: %" PRId32 " B%s",
		          stateStr,
		          svc.name.c_str(),
		          svc.id.c_str(),
		          svc.version.c_str(),
		          svc.stats.startCount,
		          svc.stats.lastStartTimeUs / 1000,
		          svc.stats.heapDeltaBytes,
		          svc.required ? " [REQUIRED]" : "");
	}

	// Apps
	Log::info(TAG, "─── Apps (%zu registered, %zu active) ───",
	          diag.appCount, diag.activeAppCount);
	for (const auto& app : diag.apps) {
		const char* state = app.isActive ? "active" : "idle";
		const char* blocked = app.isBlocked ? " [BLOCKED]" : "";
		Log::info(TAG, "  [%s] %s (%s) — launches: %" PRIu32 ", start: %" PRId64 " ms, heap: %" PRId32 " B, total: %" PRIu32 " ms%s",
		          state,
		          app.appName.c_str(),
		          app.packageName.c_str(),
		          app.stats.launchCount,
		          app.stats.lastStartTimeUs / 1000,
		          app.stats.heapDeltaBytes,
		          app.stats.totalActiveTimeMs,
		          blocked);
	}

	Log::info(TAG, "═══════════════════════════════════════════════");
}

} // namespace flx::system
