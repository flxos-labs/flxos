#include <flx/hal/BusManager.hpp>

namespace flx::hal {

BusManager::~BusManager() {
	std::lock_guard<std::mutex> guard(m_managerMutex);
	for (auto& pair : m_spiLocks) {
		if (pair.second) {
			vSemaphoreDelete(pair.second);
		}
	}
	m_spiLocks.clear();
}

bool BusManager::acquireSpi(int hostId, uint32_t timeoutMs) {
	SemaphoreHandle_t lock = nullptr;
	{
		std::lock_guard<std::mutex> guard(m_managerMutex);
		auto it = m_spiLocks.find(hostId);
		if (it == m_spiLocks.end()) {
			lock = xSemaphoreCreateRecursiveMutex();
			m_spiLocks[hostId] = lock;
		} else {
			lock = it->second;
		}
	}

	if (lock) {
		TickType_t ticks = (timeoutMs == portMAX_DELAY) ? portMAX_DELAY : pdMS_TO_TICKS(timeoutMs);
		return xSemaphoreTakeRecursive(lock, ticks) == pdTRUE;
	}
	return false;
}

void BusManager::releaseSpi(int hostId) {
	SemaphoreHandle_t lock = nullptr;
	{
		std::lock_guard<std::mutex> guard(m_managerMutex);
		auto it = m_spiLocks.find(hostId);
		if (it != m_spiLocks.end()) {
			lock = it->second;
		}
	}

	if (lock) {
		xSemaphoreGiveRecursive(lock);
	}
}

} // namespace flx::hal
