#include "BluetoothManager.hpp"
#include "Observable.hpp"
#include <flx/core/Logger.hpp>
#include "esp_err.h"
#include <cstdint>
#include <string_view>

static constexpr std::string_view TAG = "BluetoothManager";

namespace System {

esp_err_t BluetoothManager::init(Observable<int32_t>* enabled_subject) {
	Log::info(TAG, "Initializing Bluetooth Manager...");
	m_enabled_subject = enabled_subject;
	m_is_init = true;
	return ESP_OK;
}

esp_err_t BluetoothManager::enable(bool enable) {
	Log::info(TAG, "Bluetooth %s", enable ? "enabling" : "disabling");
	m_is_enabled = enable;
	if (m_enabled_subject) {
		m_enabled_subject->set(enable ? 1 : 0);
	}
	return ESP_OK;
}

bool BluetoothManager::isEnabled() const {
	return m_is_enabled;
}

} // namespace System
