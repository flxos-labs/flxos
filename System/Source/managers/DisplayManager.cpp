#include <flx/core/Logger.hpp>
#include <flx/system/managers/DisplayManager.hpp>
#include <flx/system/managers/SettingsManager.hpp>

static constexpr const char* TAG = "DisplayManager";

namespace flx::system {

const flx::services::ServiceManifest DisplayManager::serviceManifest = {
	.serviceId = "com.flxos.display",
	.serviceName = "Display",
	.dependencies = {"com.flxos.settings"},
	.priority = 20,
	.required = true,
	.autoStart = true,
	.guiRequired = false,
	.capabilities = flx::services::ServiceCapability::Display,
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

void DisplayManager::applyBrightness(int32_t val) {
	// Hardware control is handled by GuiTask via subscription to m_brightness_subject
}

void DisplayManager::applyRotation(int32_t rot) {
	// Hardware control is handled by GuiTask via subscription to m_rotation_subject
}

void DisplayManager::applyShowFps(int32_t show) {
	// FPS monitoring is handled by GuiTask via subscription to m_show_fps_subject
}

} // namespace flx::system
