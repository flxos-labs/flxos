#include <flx/core/EventBus.hpp>
#include <flx/core/Logger.hpp>
#include <flx/hal/DeviceRegistry.hpp>
#include <flx/hal/power/IPowerDevice.hpp>
#include <flx/system/managers/PowerManager.hpp>
#include <flx/system/services/SystemInfoService.hpp>

static constexpr const char* TAG = "PowerManager";

namespace flx::system {

const flx::services::ServiceManifest PowerManager::serviceManifest = {
	.serviceId = "com.flxos.power",
	.serviceName = "Power",
	.dependencies = {"com.flxos.hal"},
	.priority = 50,
	.required = false,
	.autoStart = true,
	.guiRequired = false,
	.capabilities = flx::services::ServiceCapability::None,
	.description = "Battery level, charging status, and power management",
};

bool PowerManager::onStart() {
	Log::info(TAG, "Power service starting...");

	// Try to find a HAL power device from the registry
	auto& registry = flx::hal::DeviceRegistry::getInstance();
	m_powerDevice = registry.findFirst<flx::hal::power::IPowerDevice>(flx::hal::IDevice::Type::Power);

	if (m_powerDevice) {
		Log::info(TAG, "Using HAL IPowerDevice: %.*s", static_cast<int>(m_powerDevice->getName().size()), m_powerDevice->getName().data());

		// Subscribe to power events for reactive updates
		m_halSubscriptionId = m_powerDevice->subscribePowerEvents(
			[this](flx::hal::power::IPowerDevice::PowerEvent event) {
				// Refresh observables when any power event occurs
				refresh();
				switch (event) {
					case flx::hal::power::IPowerDevice::PowerEvent::BatteryCritical:
						Log::warn(TAG, "Battery CRITICAL");
						{
							flx::core::Bundle data;
							data.putString("title", "Battery Critical");
							data.putString("message", "System will shut down soon!");
							data.putString("appName", "Power");
							data.putString("icon", "battery");
							data.putInt32("priority", 2);
							data.putString("redirectAppId", "com.flxos.settings");
							data.putString("redirectData", "power");
							flx::core::EventBus::getInstance().publish("system.notify", data);
						}
						break;
					case flx::hal::power::IPowerDevice::PowerEvent::BatteryLow:
						Log::warn(TAG, "Battery LOW");
						{
							flx::core::Bundle data;
							data.putString("title", "Battery Low");
							data.putString("message", "Please connect a charger");
							data.putString("appName", "Power");
							data.putString("icon", "battery");
							data.putInt32("priority", 2);
							data.putString("redirectAppId", "com.flxos.settings");
							data.putString("redirectData", "power");
							flx::core::EventBus::getInstance().publish("system.notify", data);
						}
						break;
					default:
						break;
				}
			});
	} else {
		Log::info(TAG, "No HAL power device found — using SystemInfoService fallback");
	}

	refresh();

	esp_timer_create_args_t const timer_args = {
		.callback = [](void* /*arg*/) {
			PowerManager::getInstance().refresh();
		},
		.arg = nullptr,
		.dispatch_method = ESP_TIMER_TASK,
		.name = "power_poll",
		.skip_unhandled_events = true,
	};
	if (esp_timer_create(&timer_args, &m_timer) == ESP_OK) {
		// Poll every 10 seconds (10,000,000 microseconds)
		esp_timer_start_periodic(m_timer, 10000000);
		Log::info(TAG, "Battery polling timer started");
	} else {
		Log::error(TAG, "Failed to create battery polling timer");
	}

	Log::info(TAG, "Power service started");
	return true;
}

void PowerManager::onStop() {
	// Stop and delete the periodic polling timer
	if (m_timer) {
		esp_timer_stop(m_timer);
		esp_timer_delete(m_timer);
		m_timer = nullptr;
	}

	// Unsubscribe from HAL events
	if (m_powerDevice && m_halSubscriptionId >= 0) {
		m_powerDevice->unsubscribePowerEvents(m_halSubscriptionId);
		m_halSubscriptionId = -1;
	}
	m_powerDevice.reset();
	Log::info(TAG, "Power service stopped");
}

void PowerManager::refresh() {
	int32_t level = 100;
	int32_t charging = 0;
	int32_t configured = 0;

	if (m_powerDevice && m_powerDevice->isHealthy()) {
		// ── HAL path: read directly from the registered power device ──
		level = static_cast<int32_t>(m_powerDevice->getChargeLevel());
		charging = m_powerDevice->isCharging() ? 1 : 0;
		configured = 1;
	} else {
		// ── Fallback path: legacy SystemInfoService ──
		auto stats = flx::services::SystemInfoService::getInstance().getBatteryStats();
		level = stats.level;
		charging = stats.isCharging ? 1 : 0;
		configured = stats.isConfigured ? 1 : 0;
	}

	if (m_batteryLevel.get() != level) {
		m_batteryLevel.set(level);
	}
	if (m_isCharging.get() != charging) {
		m_isCharging.set(charging);
	}
	if (m_isConfigured.get() != configured) {
		m_isConfigured.set(configured);
	}
}

} // namespace flx::system
