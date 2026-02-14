#include "DisplayManager.hpp"
#include <flx/core/Logger.hpp>
#include "core/system/settings/SettingsManager.hpp"

#if !CONFIG_FLXOS_HEADLESS_MODE
#include "core/tasks/gui/GuiTask.hpp"
#include "core/ui/LvglBridgeHelpers.hpp"
#include "src/debugging/sysmon/lv_sysmon.h"

#if LV_USE_LOVYAN_GFX
#include "../../../hal/display/lv_lgfx_user.hpp"
#endif
#endif

static constexpr const char* TAG = "DisplayManager";

namespace System {

const Services::ServiceManifest DisplayManager::serviceManifest = {
	.serviceId = "com.flxos.display",
	.serviceName = "Display",
	.dependencies = {"com.flxos.settings"},
	.priority = 20,
	.required = true,
	.autoStart = true,
	.guiRequired = false,
	.capabilities = Services::ServiceCapability::Display,
	.description = "Display brightness, rotation, and FPS monitoring",
};

bool DisplayManager::onStart() {
	SettingsManager::getInstance().registerSetting("brightness", m_brightness_subject);
	SettingsManager::getInstance().registerSetting("rotation", m_rotation_subject);
	SettingsManager::getInstance().registerSetting("show_fps", m_show_fps_subject);

	applyBrightness(m_brightness_subject.get());
	applyRotation(m_rotation_subject.get());
	applyShowFps(m_show_fps_subject.get());

	Log::info(TAG, "Display service started");
	return true;
}

void DisplayManager::onStop() {
	Log::info(TAG, "Display service stopped");
}

#if !CONFIG_FLXOS_HEADLESS_MODE
void DisplayManager::onGuiInit() {
	GuiTask::lock();

	INIT_INT_BRIDGE(m_brightness_bridge, m_brightness_subject, applyBrightness);
	INIT_INT_BRIDGE(m_rotation_bridge, m_rotation_subject, applyRotation);
	INIT_INT_BRIDGE(m_show_fps_bridge, m_show_fps_subject, applyShowFps);

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
		auto dsc = (lv_lovyan_gfx_driver_data_t*)lv_display_get_driver_data(d);
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
