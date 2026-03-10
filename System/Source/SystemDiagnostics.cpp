#include <flx/system/SystemDiagnostics.hpp>

#include <flx/apps/AppManager.hpp>
#include <flx/core/BootTimeline.hpp>
#include <flx/core/Logger.hpp>
#include <flx/services/ServiceRegistry.hpp>

#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static constexpr const char* TAG = "SystemDiagnostics";

namespace flx::system {

SystemDiagnostics getSystemDiagnostics() {
	SystemDiagnostics diag;

	// ─── Memory ───
	diag.memory.freeHeapBytes = esp_get_free_heap_size();
	diag.memory.minFreeHeapBytes = esp_get_minimum_free_heap_size();
	diag.memory.largestFreeBlock = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
	// ESP-IDF doesn't have a direct "total heap" API; compute from initial + allocated
	multi_heap_info_t info;
	heap_caps_get_info(&info, MALLOC_CAP_DEFAULT);
	diag.memory.totalHeapBytes = info.total_free_bytes + info.total_allocated_bytes;

	// ─── System timing ───
	diag.uptimeUs = esp_timer_get_time();
	diag.taskCount = uxTaskGetNumberOfTasks();

	// ─── Boot timeline ───
	auto& timeline = flx::core::BootTimeline::getInstance();
	diag.bootTimeUs = timeline.getTotalBootTimeUs();
	diag.bootTimelineEntries = timeline.getEntryCount();

	// ─── Services ───
	auto& registry = flx::services::ServiceRegistry::getInstance();
	const auto& allServices = registry.getAllServices();
	diag.servicesRegistered = static_cast<uint32_t>(allServices.size());
	diag.servicesRunning = 0;
	diag.servicesFailed = 0;

	for (const auto& svc : allServices) {
		if (!svc) continue;

		SystemDiagnostics::ServiceInfo si;
		const auto& manifest = svc->getManifest();

		si.id = manifest.serviceId;
		si.name = manifest.serviceName;
		si.version = manifest.version;
		si.state = svc->getState();
		si.stats = svc->getServiceStats();
		si.required = manifest.required;
		si.autoRestart = manifest.autoRestart;
		si.groups = manifest.groups;

		// Run a non-invasive health query for running services
		if (svc->isRunning()) {
			si.lastHealthStatus = svc->onHealthCheck();
			diag.servicesRunning++;
		} else if (si.state == flx::services::ServiceState::Failed) {
			si.lastHealthStatus = flx::services::HealthStatus::Critical;
			diag.servicesFailed++;
		}

		diag.services.push_back(std::move(si));
	}

	// ─── Apps ───
	auto& appMgr = flx::apps::AppManager::getInstance();
	const auto& installedApps = appMgr.getInstalledApps();
	diag.appsRegistered = static_cast<uint32_t>(installedApps.size());
	diag.appsInStack = static_cast<uint32_t>(appMgr.getStackDepth());

	auto currentApp = appMgr.getCurrentApp();

	for (const auto& app : installedApps) {
		if (!app) continue;

		SystemDiagnostics::AppInfo ai;
		ai.packageName = app->getPackageName();
		ai.appName = app->getAppName();
		ai.isActive = appMgr.isAppInStack(ai.packageName);
		ai.isForeground = (currentApp && currentApp->getPackageName() == ai.packageName);
		ai.stats = appMgr.getAppStats(ai.packageName);
		ai.isBlocked = appMgr.isAppBlocked(ai.packageName);

		diag.apps.push_back(std::move(ai));
	}

	return diag;
}

void dumpSystemDiagnostics() {
	auto diag = getSystemDiagnostics();

	Log::info(TAG, "╔══════════════════════════════════════════════╗");
	Log::info(TAG, "║          System Diagnostics Report           ║");
	Log::info(TAG, "╠══════════════════════════════════════════════╣");

	// Memory
	Log::info(TAG, "║ Memory                                      ║");
	Log::info(TAG, "║  Free heap:     %7lu KB / %7lu KB       ║",
	          (unsigned long)(diag.memory.freeHeapBytes / 1024),
	          (unsigned long)(diag.memory.totalHeapBytes / 1024));
	Log::info(TAG, "║  Min free:      %7lu KB                   ║",
	          (unsigned long)(diag.memory.minFreeHeapBytes / 1024));
	Log::info(TAG, "║  Largest block: %7lu KB                   ║",
	          (unsigned long)(diag.memory.largestFreeBlock / 1024));

	// System
	Log::info(TAG, "╠──────────────────────────────────────────────╣");
	Log::info(TAG, "║ System                                      ║");
	Log::info(TAG, "║  Uptime:        %lld s                      ║", (long long)(diag.uptimeUs / 1000000));
	Log::info(TAG, "║  Boot time:     %lld ms (%zu events)        ║",
	          (long long)(diag.bootTimeUs / 1000), diag.bootTimelineEntries);
	Log::info(TAG, "║  Tasks:         %zu                          ║", diag.taskCount);

	// Services
	Log::info(TAG, "╠──────────────────────────────────────────────╣");
	Log::info(TAG, "║ Services (%u registered, %u running, %u failed) ║",
	          diag.servicesRegistered, diag.servicesRunning, diag.servicesFailed);

	for (const auto& svc : diag.services) {
		const char* stateStr = flx::services::serviceStateToString(svc.state);
		const char* healthIcon = "✓";
		if (svc.lastHealthStatus == flx::services::HealthStatus::Degraded) healthIcon = "⚠";
		else if (svc.lastHealthStatus == flx::services::HealthStatus::Unhealthy) healthIcon = "✗";
		else if (svc.lastHealthStatus == flx::services::HealthStatus::Critical) healthIcon = "☠";

		Log::info(TAG, "║  %s %-28s [%s] v%s  starts=%lu  boot=%lld ms  heap=%+ld B",
		          healthIcon,
		          svc.name.c_str(),
		          stateStr,
		          svc.version.c_str(),
		          (unsigned long)svc.stats.startCount,
		          (long long)(svc.stats.lastStartTimeUs / 1000),
		          (long)svc.stats.heapDeltaBytes);
	}

	// Apps
	Log::info(TAG, "╠──────────────────────────────────────────────╣");
	Log::info(TAG, "║ Apps (%u registered, %u in stack)             ║",
	          diag.appsRegistered, diag.appsInStack);

	for (const auto& app : diag.apps) {
		const char* state = "idle";
		if (app.isBlocked) state = "BLOCKED";
		else if (app.isForeground) state = "fg";
		else if (app.isActive) state = "bg";

		Log::info(TAG, "║  [%-7s] %-24s launches=%lu  active=%lu ms",
		          state,
		          app.appName.c_str(),
		          (unsigned long)app.stats.launchCount,
		          (unsigned long)app.stats.totalActiveTimeMs);
	}

	Log::info(TAG, "╚══════════════════════════════════════════════╝");
}

} // namespace flx::system
