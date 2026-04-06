#include <Config.hpp>
#include <flx/core/EventBus.hpp>
#include <flx/core/Logger.hpp>
#include <flx/hal/DeviceRegistry.hpp>
#include <flx/hal/sdcard/SdmmcSdCardDevice.hpp>
#include <flx/hal/sdcard/SpiSdCardDevice.hpp>
#include <flx/system/services/SdCardService.hpp>
#include <sdkconfig.h>
#include <string>
#include <string_view>

namespace flx::services {

static constexpr std::string_view TAG = "SdCardService";

const ServiceManifest SdCardService::serviceManifest = {
	.serviceId = "com.flxos.sdcard",
	.serviceName = "SD Card",
	.dependencies = {},
	.priority = 15,
	.required = false,
	.autoStart = true,
	.guiRequired = false,
	.capabilities = ServiceCapability::Storage,
	.description = "SD card mount/unmount wrapper",
};

SdCardService& SdCardService::getInstance() {
	static SdCardService instance;
	return instance;
}

bool SdCardService::onStart() {
	auto& registry = flx::hal::DeviceRegistry::getInstance();

	std::string_view bus_type = flx::config::sdcard.bus;

	if (bus_type == "sdmmc") {
		Log::info(TAG, "Starting native SDMMC HAL device...");
		auto sdmmc = std::make_shared<flx::hal::sdcard::SdmmcSdCardDevice>();
		if (sdmmc->start()) {
			registry.registerDevice(sdmmc);
			m_device = sdmmc;
		} else {
			Log::warn(TAG, "Failed to start native SDMMC HAL device");
		}
	} else {
		// Default to SPI if not specified or explicit "spi"
		Log::info(TAG, "Starting SPI SD card HAL device...");
		auto spi = std::make_shared<flx::hal::sdcard::SpiSdCardDevice>();
		if (spi->start()) {
			registry.registerDevice(spi);
			m_device = spi;
		} else {
			Log::warn(TAG, "Failed to start SPI SD card HAL device");
		}
	}

	if (!m_device) {
		return false;
	}

	// Mount it
	const bool mounted = m_device->mount(flx::config::sdcard.mountPoint);
	if (mounted) {
		Log::info(TAG, "Mounted SD card via HAL (%s)", bus_type.data());
		flx::hal::sdcard::ISdCardDevice::CardInfo info;
		if (m_device->getCardInfo(info)) {
			Log::info(TAG, "SD Card Info: Size: %llu MB, Free: %llu MB, FS: %s", (unsigned long long)(info.totalBytes / (1024ULL * 1024ULL)), (unsigned long long)(info.freeBytes / (1024ULL * 1024ULL)), info.fsType.c_str());
		}
	} else {
		Log::warn(TAG, "Failed to mount SD card via HAL (%s)", bus_type.data());
	}

	return mounted;
}

void SdCardService::onGuiInit() {
	if (isMounted()) {
		flx::core::Bundle data;
		data.putString("title", "Storage");
		data.putString("message", "SD Card mounted at " + getMountPoint());
		data.putString("appName", "System");
		data.putString("icon", "info");
		flx::core::EventBus::getInstance().publish("system.notify", data);
	}
}

void SdCardService::onStop() {
	if (m_device) {
		m_device->unmount();
		flx::hal::DeviceRegistry::getInstance().deregisterDevice(m_device->getId());
		m_device->stop();
		m_device.reset();
	}
}

bool SdCardService::isMounted() const {
	return m_device && m_device->isMounted();
}

std::string SdCardService::getMountPoint() const {
	return m_device ? m_device->getMountPath() : flx::config::sdcard.mountPoint;
}

} // namespace flx::services
