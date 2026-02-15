#include <flx/core/Logger.hpp>
#include <flx/system/managers/PowerManager.hpp>
#include <flx/system/services/SystemInfoService.hpp>

static constexpr const char* TAG = "PowerManager";

namespace flx::system {

const flx::services::ServiceManifest PowerManager::serviceManifest = {
	.serviceId = "com.flxos.power",
	.serviceName = "Power",
	.dependencies = {},
	.priority = 50,
	.required = false,
	.autoStart = true,
	.guiRequired = false,
	.capabilities = flx::services::ServiceCapability::None,
	.description = "Battery level, charging status, and power management",
};

bool PowerManager::onStart() {
	Log::info(TAG, "Power service starting...");
	refresh();
	Log::info(TAG, "Power service started");
	return true;
}

void PowerManager::onStop() {
	Log::info(TAG, "Power service stopped");
}

void PowerManager::refresh() {
	auto stats = flx::services::SystemInfoService::getInstance().getBatteryStats();

	if (m_batteryLevel.get() != stats.level) {
		m_batteryLevel.set(stats.level);
	}

	int32_t is_charging = stats.isCharging ? 1 : 0;
	if (m_isCharging.get() != is_charging) {
		m_isCharging.set(is_charging);
	}

	int32_t is_configured = stats.isConfigured ? 1 : 0;
	if (m_isConfigured.get() != is_configured) {
		m_isConfigured.set(is_configured);
	}
}

} // namespace flx::system
