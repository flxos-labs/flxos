#pragma once

#include "Config.hpp"
#include "sdkconfig.h"
#include <flx/services/IService.hpp>
#include <flx/services/ServiceManifest.hpp>
#include <string>

#if FLXOS_SD_CARD_ENABLED
#include "sdmmc_cmd.h"
#endif

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

	bool isMounted() const { return m_mounted; }
	const std::string& getMountPoint() const { return m_mountPoint; }

private:

	SdCardService();
	~SdCardService() = default;

	bool mount();
	void unmount();

	bool m_mounted = false;
	bool m_spiOwner = false; // true if we called spi_bus_initialize

	std::string m_mountPoint;

#if FLXOS_SD_CARD_ENABLED
	sdmmc_card_t* m_card = nullptr;
#endif
};

} // namespace flx::services
