#pragma once

#include "esp_err.h"
#include "lvgl.h"

namespace System {

class BluetoothManager {
public:

	static BluetoothManager& getInstance();

	esp_err_t init(lv_subject_t* enabled_subject);
	esp_err_t enable(bool enable);
	bool isEnabled() const;

private:

	BluetoothManager() = default;
	~BluetoothManager() = default;

	lv_subject_t* m_enabled_subject = nullptr;
	bool m_is_init = false;
};

} // namespace System
