#pragma once

#include <flx/core/Observable.hpp>
#include <flx/core/Singleton.hpp>
#include "core/services/IService.hpp"
#include "core/services/ServiceManifest.hpp"
#include <memory>

#if !CONFIG_FLXOS_HEADLESS_MODE
#include "core/ui/LvglObserverBridge.hpp"
#include "lvgl.h"
#endif

namespace System {

class DisplayManager : public flx::Singleton<DisplayManager>, public Services::IService {
	friend class flx::Singleton<DisplayManager>;

public:

	// ──── IService manifest ────
	static const Services::ServiceManifest serviceManifest;
	const Services::ServiceManifest& getManifest() const override { return serviceManifest; }

	// ──── IService lifecycle ────
	bool onStart() override;
	void onStop() override;

#if !CONFIG_FLXOS_HEADLESS_MODE
	void onGuiInit() override;
#endif

	flx::Observable<int32_t>& getBrightnessObservable() { return m_brightness_subject; }
	flx::Observable<int32_t>& getRotationObservable() { return m_rotation_subject; }
	flx::Observable<int32_t>& getShowFpsObservable() { return m_show_fps_subject; }

#if !CONFIG_FLXOS_HEADLESS_MODE
	lv_subject_t& getBrightnessSubject() { return *m_brightness_bridge->getSubject(); }
	lv_subject_t& getRotationSubject() { return *m_rotation_bridge->getSubject(); }
	lv_subject_t& getShowFpsSubject() { return *m_show_fps_bridge->getSubject(); }
#endif

private:

	DisplayManager() = default;
	~DisplayManager() = default;

	flx::Observable<int32_t> m_brightness_subject {127};
	flx::Observable<int32_t> m_rotation_subject {90};
	flx::Observable<int32_t> m_show_fps_subject {0};

#if !CONFIG_FLXOS_HEADLESS_MODE
	std::unique_ptr<LvglObserverBridge<int32_t>> m_brightness_bridge {};
	std::unique_ptr<LvglObserverBridge<int32_t>> m_rotation_bridge {};
	std::unique_ptr<LvglObserverBridge<int32_t>> m_show_fps_bridge {};
#endif

	void applyBrightness(int32_t val);
	void applyRotation(int32_t rot);
	void applyShowFps(int32_t show);
};

} // namespace System
