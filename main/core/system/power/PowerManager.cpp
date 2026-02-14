#include "PowerManager.hpp"
#include "core/services/system_info/SystemInfoService.hpp"
#include "core/ui/LvglBridgeHelpers.hpp"
#include <flx/core/Logger.hpp>

#if !CONFIG_FLXOS_HEADLESS_MODE
#include "core/tasks/gui/GuiTask.hpp"
#endif

static constexpr const char* TAG = "PowerManager";

namespace System {

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

#if !CONFIG_FLXOS_HEADLESS_MODE
void PowerManager::onGuiInit() {
	Log::info(TAG, "Initializing GUI bridges...");
	INIT_BRIDGE(m_batteryLevelBridge, m_batteryLevel);
	INIT_BRIDGE(m_isChargingBridge, m_isCharging);
}
#endif

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

} // namespace System
