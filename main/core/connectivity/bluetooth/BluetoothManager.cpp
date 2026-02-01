#include "BluetoothManager.hpp"
#include "core/common/Logger.hpp"
#include "core/tasks/gui/GuiTask.hpp"
#include <string_view>

static constexpr std::string_view TAG = "BluetoothManager";

namespace System {

BluetoothManager& BluetoothManager::getInstance() {
	static BluetoothManager instance;
	return instance;
}

esp_err_t BluetoothManager::init(lv_subject_t* enabled_subject) {
	Log::info(TAG, "Initializing Bluetooth Manager...");
	m_enabled_subject = enabled_subject;
	m_is_init = true;
	return ESP_OK;
}

esp_err_t BluetoothManager::enable(bool enable) {
	Log::info(TAG, "Bluetooth %s", enable ? "enabling" : "disabling");
	GuiTask::lock();
	if (m_enabled_subject)
		lv_subject_set_int(m_enabled_subject, enable ? 1 : 0);
	GuiTask::unlock();
	return ESP_OK;
}

bool BluetoothManager::isEnabled() const {
	GuiTask::lock();
	bool enabled = false;
	if (m_enabled_subject)
		enabled = lv_subject_get_int(m_enabled_subject) != 0;
	GuiTask::unlock();
	return enabled;
}

} // namespace System
