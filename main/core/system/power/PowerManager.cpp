#include "PowerManager.hpp"
#include "core/common/Logger.hpp"
#include "core/services/system_info/SystemInfoService.hpp"

static constexpr const char* TAG = "PowerManager";

namespace System {

PowerManager& PowerManager::getInstance() {
	static PowerManager instance;
	return instance;
}

void PowerManager::init() {
	if (m_is_init) return;
	Log::info(TAG, "Initializing PowerManager...");
	refresh();
	m_is_init = true;
}

#if !CONFIG_FLXOS_HEADLESS_MODE
void PowerManager::initGuiBridges() {
	Log::info(TAG, "Initializing GUI bridges...");
	m_batteryLevelBridge = std::make_unique<LvglObserverBridge<int32_t>>(m_batteryLevel);
	m_isChargingBridge = std::make_unique<LvglObserverBridge<int32_t>>(m_isCharging);
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
