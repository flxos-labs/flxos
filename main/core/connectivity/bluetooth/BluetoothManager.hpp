#pragma once

#include "core/common/Observable.hpp"
#include "esp_err.h"

namespace System {

class BluetoothManager {
public:

	static BluetoothManager& getInstance();

	esp_err_t init(Observable<int32_t>* enabled_subject);
	esp_err_t enable(bool enable);
	bool isEnabled() const;

private:

	BluetoothManager() = default;
	~BluetoothManager() = default;

	Observable<int32_t>* m_enabled_subject = nullptr;
	bool m_is_init = false;
	bool m_is_enabled = false;
};

} // namespace System
