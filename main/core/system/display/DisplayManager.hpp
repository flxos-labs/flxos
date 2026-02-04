#pragma once

#include "core/common/Observable.hpp"
#include <memory>

#if !CONFIG_FLXOS_HEADLESS_MODE
#include "core/ui/LvglObserverBridge.hpp"
#include "lvgl.h"
#endif

namespace System {

class DisplayManager {
public:

	static DisplayManager& getInstance();

	void init();

#if !CONFIG_FLXOS_HEADLESS_MODE
	void initGuiBridges();
#endif

	Observable<int32_t>& getBrightnessObservable() { return m_brightness_subject; }
	Observable<int32_t>& getRotationObservable() { return m_rotation_subject; }
	Observable<int32_t>& getShowFpsObservable() { return m_show_fps_subject; }

#if !CONFIG_FLXOS_HEADLESS_MODE
	lv_subject_t& getBrightnessSubject() { return *m_brightness_bridge->getSubject(); }
	lv_subject_t& getRotationSubject() { return *m_rotation_bridge->getSubject(); }
	lv_subject_t& getShowFpsSubject() { return *m_show_fps_bridge->getSubject(); }
#endif

private:

	DisplayManager() = default;
	~DisplayManager() = default;
	DisplayManager(const DisplayManager&) = delete;
	DisplayManager& operator=(const DisplayManager&) = delete;

	Observable<int32_t> m_brightness_subject {127};
	Observable<int32_t> m_rotation_subject {90};
	Observable<int32_t> m_show_fps_subject {0};

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
