#include "PowerManager.hpp"
#include "core/common/Logger.hpp"
#include "core/services/system_info/SystemInfoService.hpp"
#include "core/ui/LvglBridgeHelpers.hpp"

#if !CONFIG_FLXOS_HEADLESS_MODE
#include "core/tasks/gui/GuiTask.hpp"
#endif

static constexpr const char* TAG = "PowerManager";

namespace System {

void PowerManager::init() {
	if (m_is_init) return;
	Log::info(TAG, "Initializing PowerManager...");
	refresh();
	m_is_init = true;
}

#if !CONFIG_FLXOS_HEADLESS_MODE
void PowerManager::initGuiBridges() {
	Log::info(TAG, "Initializing GUI bridges...");
	INIT_BRIDGE(m_batteryLevelBridge, m_batteryLevel);
	INIT_BRIDGE(m_isChargingBridge, m_isCharging);
}
#endif

void PowerManager::refresh() {
	auto stats = Services::SystemInfoService::getInstance().getBatteryStats();

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
