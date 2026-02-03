#include "DisplayManager.hpp"
#include "core/common/Logger.hpp"
#include "core/system/Settings/SettingsManager.hpp"

#if !CONFIG_FLXOS_HEADLESS_MODE
#include "core/tasks/gui/GuiTask.hpp"
#include "src/debugging/sysmon/lv_sysmon.h"

#if LV_USE_LOVYAN_GFX
#include "../../../hal/display/lv_lgfx_user.hpp"
typedef struct {
	LGFX* tft;
} lv_lovyan_gfx_t;
#endif
#endif

static constexpr const char* TAG = "DisplayManager";

namespace System {

DisplayManager& DisplayManager::getInstance() {
	static DisplayManager instance;
	return instance;
}

void DisplayManager::init() {
	SettingsManager::getInstance().registerSetting("brightness", m_brightness_subject);
	SettingsManager::getInstance().registerSetting("rotation", m_rotation_subject);
	SettingsManager::getInstance().registerSetting("show_fps", m_show_fps_subject);

	// Initial application of settings (if GUI is running, it will be done in initGuiBridges or via subscriptions)
	applyBrightness(m_brightness_subject.get());
	applyRotation(m_rotation_subject.get());
	applyShowFps(m_show_fps_subject.get());
}

#if !CONFIG_FLXOS_HEADLESS_MODE
void DisplayManager::initGuiBridges() {
	GuiTask::lock();
	m_brightness_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_brightness_subject);
	m_rotation_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_rotation_subject);
	m_show_fps_bridge = std::make_unique<LvglObserverBridge<int32_t>>(m_show_fps_subject);

	lv_subject_add_observer(m_brightness_bridge->getSubject(), [](lv_observer_t*, lv_subject_t* s) { DisplayManager::getInstance().applyBrightness(lv_subject_get_int(s)); }, nullptr);

	lv_subject_add_observer(m_rotation_bridge->getSubject(), [](lv_observer_t*, lv_subject_t* s) { DisplayManager::getInstance().applyRotation(lv_subject_get_int(s)); }, nullptr);

	lv_subject_add_observer(m_show_fps_bridge->getSubject(), [](lv_observer_t*, lv_subject_t* s) { DisplayManager::getInstance().applyShowFps(lv_subject_get_int(s)); }, nullptr);

	// Apply initial values to GUI
	applyBrightness(m_brightness_subject.get());
	applyRotation(m_rotation_subject.get());
	applyShowFps(m_show_fps_subject.get());

	GuiTask::unlock();
}
#endif

void DisplayManager::applyBrightness(int32_t val) {
#if !CONFIG_FLXOS_HEADLESS_MODE
	if (auto d = lv_display_get_default()) {
#if LV_USE_LOVYAN_GFX
		auto dsc = (lv_lovyan_gfx_t*)lv_display_get_driver_data(d);
		if (dsc && dsc->tft)
			dsc->tft->setBrightness((uint8_t)val);
#endif
	}
#endif
}

void DisplayManager::applyRotation(int32_t rot) {
#if !CONFIG_FLXOS_HEADLESS_MODE
	if (auto d = lv_display_get_default())
		lv_display_set_rotation(d, (lv_display_rotation_t)(rot / 90));
#endif
}

void DisplayManager::applyShowFps(int32_t show) {
#if !CONFIG_FLXOS_HEADLESS_MODE && LV_USE_SYSMON && LV_USE_PERF_MONITOR
	if (show) {
		lv_sysmon_show_performance(lv_display_get_default());
	} else {
		lv_sysmon_hide_performance(lv_display_get_default());
	}
#endif
}

} // namespace System
