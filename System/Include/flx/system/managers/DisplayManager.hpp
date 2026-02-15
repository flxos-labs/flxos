#pragma once

#include <flx/core/Observable.hpp>
#include <flx/core/Singleton.hpp>
#include <flx/services/IService.hpp>
#include <flx/services/ServiceManifest.hpp>
#include <memory>

namespace flx::system {

class DisplayManager : public flx::Singleton<DisplayManager>, public flx::services::IService {
	friend class flx::Singleton<DisplayManager>;

public:

	// ──── IService manifest ────
	static const flx::services::ServiceManifest serviceManifest;
	const flx::services::ServiceManifest& getManifest() const override { return serviceManifest; }

	// ──── IService lifecycle ────
	bool onStart() override;
	void onStop() override;

	flx::Observable<int32_t>& getBrightnessObservable() { return m_brightness_subject; }
	flx::Observable<int32_t>& getRotationObservable() { return m_rotation_subject; }
	flx::Observable<int32_t>& getShowFpsObservable() { return m_show_fps_subject; }

private:

	DisplayManager() = default;
	~DisplayManager() = default;

	flx::Observable<int32_t> m_brightness_subject {127};
	flx::Observable<int32_t> m_rotation_subject {90};
	flx::Observable<int32_t> m_show_fps_subject {0};

	void applyBrightness(int32_t val);
	void applyRotation(int32_t rot);
	void applyShowFps(int32_t show);
};

} // namespace flx::system
