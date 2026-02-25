#include <flx/hal/DeviceBase.hpp>

namespace flx::hal {

// Out-of-line definition for generating unique device IDs.
IDevice::Id _generateDeviceId() {
	static std::atomic<IDevice::Id> s_nextId {1};
	return s_nextId.fetch_add(1, std::memory_order_relaxed);
}

} // namespace flx::hal
