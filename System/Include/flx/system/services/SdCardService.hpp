#pragma once

#include "Config.hpp"
#include "sdkconfig.h"
#include <flx/hal/sdcard/ISdCardDevice.hpp>
#include <flx/services/IService.hpp>
#include <flx/services/ServiceManifest.hpp>
#include <memory>
#include <string>

namespace flx::services {

class SdCardService : public IService {
public:

	static SdCardService& getInstance();

	// ──── IService manifest ────
	static const ServiceManifest serviceManifest;
	const ServiceManifest& getManifest() const override { return serviceManifest; }

	// ──── IService lifecycle ────
	bool onStart() override;
	void onStop() override;
	void onGuiInit() override;

	bool isMounted() const;
	std::string getMountPoint() const;

private:

	SdCardService() = default;
	~SdCardService() = default;

	std::shared_ptr<flx::hal::sdcard::ISdCardDevice> m_device;
};

} // namespace flx::services
