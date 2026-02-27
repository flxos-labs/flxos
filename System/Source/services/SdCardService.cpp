#include "sdkconfig.h"
#include <Config.hpp>
#include <flx/core/Logger.hpp>
#include <flx/hal/DeviceRegistry.hpp>
#include <flx/hal/sdcard/SpiSdCardDevice.hpp>
#include <flx/system/services/SdCardService.hpp>
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

	// Create and start SPI SD card HAL device
	auto sdcard = std::make_shared<flx::hal::sdcard::SpiSdCardDevice>();
	registry.registerDevice(sdcard);
	m_device = sdcard;

	if (!m_device->start()) {
		Log::warn(TAG, "Failed to start SDK card HAL device");
		return false;
	}

	// Mount it
	const bool mounted = m_device->mount(flx::config::sdcard.mountPoint);
	if (mounted) {
		Log::info(TAG, "Mounted SD card via HAL");
		flx::hal::sdcard::ISdCardDevice::CardInfo info;
		if (m_device->getCardInfo(info)) {
			Log::info(TAG, "SD Card Info: Size: %llu MB, Free: %llu MB, FS: %s", (unsigned long long)(info.totalBytes / (1024ULL * 1024ULL)), (unsigned long long)(info.freeBytes / (1024ULL * 1024ULL)), info.fsType.c_str());
		}
	} else {
		Log::warn(TAG, "Failed to mount SD card via HAL");
	}

	return mounted;
}

void SdCardService::onGuiInit() {}

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
