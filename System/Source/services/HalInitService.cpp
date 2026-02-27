#include <flx/system/services/HalInitService.hpp>
#include <flx/core/Logger.hpp>
#include <flx/hal/DeviceRegistry.hpp>
#include <flx/services/ServiceRegistry.hpp>
#include <flx/core/EventBus.hpp>

extern "C" esp_err_t flx_profile_hwd_init();

namespace flx::system::services {

static constexpr std::string_view TAG = "HalInitService";

const flx::services::ServiceManifest HalInitService::serviceManifest = {
	.serviceId = "com.flxos.hal",
	.serviceName = "HAL",
	.dependencies = {},
	.priority = 5,
	.required = true,
	.autoStart = true,
	.guiRequired = false,
};

HalInitService& HalInitService::getInstance() {
	static HalInitService instance;
	return instance;
}

bool HalInitService::onStart() {
	flx::Log::info(TAG, "Initializing hardware via generated topology...");

	auto& registry = flx::hal::DeviceRegistry::getInstance();
	
	// Wire HAL lifecycle transitions into EventBus
	[[maybe_unused]] static int subId = registry.subscribe([](const std::shared_ptr<flx::hal::IDevice>& device, bool added) {
		flx::core::Bundle data;
		data.putInt32("id", device->getId());
		data.putString("type", flx::hal::IDevice::typeToString(device->getType()));
		data.putString("name", std::string(device->getName()));
		
		if (added) {
			flx::core::EventBus::getInstance().publish("hal.device.registered", data);
		} else {
			flx::core::EventBus::getInstance().publish("hal.device.removed", data);
		}
	});

	flx::core::EventBus::getInstance().publish("hal.init.begin", {});

	esp_err_t err = flx_profile_hwd_init();
	if (err != ESP_OK) {
		flx::Log::error(TAG, "Hardware initialization failed: %d", err);
		return false;
	}

	flx::Log::info(TAG, "Hardware initialization complete.");
	
	// Report registered devices
	flx::Log::info(TAG, "Registered %zu HAL devices.", registry.count());

	flx::core::Bundle completeData;
	completeData.putInt32("deviceCount", registry.count());
	flx::core::EventBus::getInstance().publish("hal.init.complete", completeData);

	return true;
}

void HalInitService::onStop() {
	flx::Log::info(TAG, "Stopping hardware services (no-op)");
}

void HalInitService::onHealthCheck() {
	auto& registry = flx::hal::DeviceRegistry::getInstance();
	auto report = registry.getHealthReport();
	
	if (report.errorDevices > 0) {
		flx::Log::warn(TAG, "Health check found %zu devices in error state.", report.errorDevices);
		for (const auto& dev : report.unhealthyDevices) {
			flx::Log::warn(TAG, "Device ID %lu is unhealthy", dev.first);
		}
	}
}

} // namespace flx::system::services
