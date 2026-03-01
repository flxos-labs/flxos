#pragma once

#include <flx/services/IService.hpp>
#include <flx/services/ServiceManifest.hpp>

namespace flx::system::services {

class HalInitService : public flx::services::IService {
public:

	static HalInitService& getInstance();

	static const flx::services::ServiceManifest serviceManifest;
	const flx::services::ServiceManifest& getManifest() const override { return serviceManifest; }

	bool onStart() override;
	void onStop() override;
	void onHealthCheck() override;

private:

	HalInitService() = default;
};

} // namespace flx::system::services
