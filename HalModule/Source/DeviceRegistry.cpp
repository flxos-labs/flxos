#include <algorithm>
#include <flx/core/Logger.hpp>
#include <flx/hal/DeviceRegistry.hpp>

static constexpr const char* TAG = "DeviceRegistry";

namespace flx::hal {

// ── Registration ──────────────────────────────────────────────────────────

void DeviceRegistry::registerDevice(std::shared_ptr<IDevice> device) {
	if (!device) {
		flx::Log::warn(TAG, "registerDevice called with null device, ignoring");
		return;
	}

	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		// Check for duplicate ID
		for (const auto& d: m_devices) {
			if (d->getId() == device->getId()) {
				flx::Log::warn(TAG, "Device ID %" PRIu32 " already registered, skipping", device->getId());
				return;
			}
		}
		m_devices.push_back(device);
		flx::Log::info(TAG, "Registered [%s] '%.*s' (id=%" PRIu32 ", state=%s)", IDevice::typeToString(device->getType()), (int)device->getName().size(), device->getName().data(), device->getId(), IDevice::stateToString(device->getState()));
	}

	// Notify observers outside the lock to prevent deadlocks
	notifyObservers(device, true);
}

void DeviceRegistry::deregisterDevice(IDevice::Id id) {
	std::shared_ptr<IDevice> removed;

	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		auto it = std::find_if(m_devices.begin(), m_devices.end(), [id](const auto& d) { return d->getId() == id; });

		if (it == m_devices.end()) {
			flx::Log::warn(TAG, "deregisterDevice: ID %" PRIu32 " not found", id);
			return;
		}

		removed = *it;
		m_devices.erase(it);
		flx::Log::info(TAG, "Deregistered [%s] '%.*s' (id=%" PRIu32 ")", IDevice::typeToString(removed->getType()), (int)removed->getName().size(), removed->getName().data(), removed->getId());
	}

	notifyObservers(removed, false);
}

// ── Queries ───────────────────────────────────────────────────────────────

std::shared_ptr<IDevice> DeviceRegistry::findById(IDevice::Id id) const {
	std::lock_guard<std::recursive_mutex> lock(m_mutex);
	for (const auto& d: m_devices) {
		if (d->getId() == id) return d;
	}
	return nullptr;
}

std::shared_ptr<IDevice> DeviceRegistry::findByName(std::string_view name) const {
	std::lock_guard<std::recursive_mutex> lock(m_mutex);
	for (const auto& d: m_devices) {
		if (d->getName() == name) return d;
	}
	return nullptr;
}

std::vector<std::shared_ptr<IDevice>> DeviceRegistry::findByType(IDevice::Type type) const {
	std::lock_guard<std::recursive_mutex> lock(m_mutex);
	std::vector<std::shared_ptr<IDevice>> result;
	for (const auto& d: m_devices) {
		if (d->getType() == type) {
			result.push_back(d);
		}
	}
	return result;
}

std::vector<std::shared_ptr<IDevice>> DeviceRegistry::getAll() const {
	std::lock_guard<std::recursive_mutex> lock(m_mutex);
	return m_devices;
}

bool DeviceRegistry::hasDevice(IDevice::Type type) const {
	std::lock_guard<std::recursive_mutex> lock(m_mutex);
	for (const auto& d: m_devices) {
		if (d->getType() == type) return true;
	}
	return false;
}

size_t DeviceRegistry::count() const {
	std::lock_guard<std::recursive_mutex> lock(m_mutex);
	return m_devices.size();
}

// ── Observers ─────────────────────────────────────────────────────────────

int DeviceRegistry::subscribe(DeviceChangeCallback callback) {
	std::lock_guard<std::recursive_mutex> lock(m_mutex);
	int id = m_nextSubscriptionId++;
	m_observers.emplace_back(id, std::move(callback));
	return id;
}

void DeviceRegistry::unsubscribe(int subscriptionId) {
	std::lock_guard<std::recursive_mutex> lock(m_mutex);
	m_observers.erase(
		std::remove_if(m_observers.begin(), m_observers.end(), [subscriptionId](const auto& pair) { return pair.first == subscriptionId; }),
		m_observers.end()
	);
}

void DeviceRegistry::notifyObservers(const std::shared_ptr<IDevice>& device, bool added) {
	// Snapshot observers to avoid holding the lock during callbacks
	std::vector<std::pair<int, DeviceChangeCallback>> snapshot;
	{
		std::lock_guard<std::recursive_mutex> lock(m_mutex);
		snapshot = m_observers;
	}
	for (auto& [id, cb]: snapshot) {
		if (cb) cb(device, added);
	}
}

// ── Health ────────────────────────────────────────────────────────────────

DeviceRegistry::HealthReport DeviceRegistry::getHealthReport() const {
	std::lock_guard<std::recursive_mutex> lock(m_mutex);
	HealthReport report;
	report.totalDevices = m_devices.size();

	for (const auto& d: m_devices) {
		if (d->isHealthy()) {
			report.healthyDevices++;
		} else {
			report.errorDevices++;
			report.unhealthyDevices.emplace_back(d->getId(), d->getState());
		}
	}
	return report;
}

// ── Debug ─────────────────────────────────────────────────────────────────

void DeviceRegistry::dumpDevices() const {
	std::lock_guard<std::recursive_mutex> lock(m_mutex);
	flx::Log::info(TAG, "=== HAL Device Registry (%zu devices) ===", m_devices.size());
	for (const auto& d: m_devices) {
		flx::Log::info(TAG, "  [%2" PRIu32 "] %-10s %-24.*s %s", d->getId(), IDevice::typeToString(d->getType()), (int)d->getName().size(), d->getName().data(), IDevice::stateToString(d->getState()));
	}
}

} // namespace flx::hal
