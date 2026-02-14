#pragma once

#include <flx/services/IService.hpp>
#include <flx/services/ServiceManifest.hpp>
#include "sdkconfig.h"
#include <string>

#if defined(CONFIG_FLXOS_SD_CARD_ENABLED)
struct sdmmc_card_t;
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

	bool isMounted() const { return m_mounted; }
	const std::string& getMountPoint() const { return m_mountPoint; }

private:

	SdCardService();
	~SdCardService() = default;

	bool mount();
	void unmount();

	bool m_mounted = false;
	std::string m_mountPoint;

#if defined(CONFIG_FLXOS_SD_CARD_ENABLED)
	sdmmc_card_t* m_card = nullptr;
	bool m_busInitializedHere = false;
#endif
};

} // namespace flx::services
