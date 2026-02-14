#pragma once

#include <flx/core/Observable.hpp>
#include <flx/core/Singleton.hpp>
#include "esp_err.h"

namespace System {

class BluetoothManager : public flx::Singleton<BluetoothManager> {
	friend class flx::Singleton<BluetoothManager>;

public:

	esp_err_t init(flx::Observable<int32_t>* enabled_subject);
	esp_err_t enable(bool enable);
	bool isEnabled() const;

private:

	BluetoothManager() = default;
	~BluetoothManager() = default;

	flx::Observable<int32_t>* m_enabled_subject = nullptr;
	bool m_is_init = false;
	bool m_is_enabled = false;
};

} // namespace System
