#include "BluetoothManager.hpp"
#include "core/tasks/gui/GuiTask.hpp"
#include "esp_log.h"

static const char* TAG = "BluetoothManager";

namespace System {

BluetoothManager& BluetoothManager::getInstance() {
	static BluetoothManager instance;
	return instance;
}

esp_err_t BluetoothManager::init(lv_subject_t* enabled_subject) {
	m_enabled_subject = enabled_subject;
	m_is_init = true;
	return ESP_OK;
}

esp_err_t BluetoothManager::enable(bool enable) {
	ESP_LOGW(TAG, "Bluetooth not yet implemented");
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
