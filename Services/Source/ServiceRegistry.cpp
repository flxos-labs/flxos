#include <flx/services/ServiceRegistry.hpp>
#include <flx/core/Logger.hpp>
#include <algorithm>
#include <queue>
#include <unordered_set>

static constexpr const char* TAG = "ServiceRegistry";

namespace flx::services {

// Event names for service lifecycle
namespace Events {
static constexpr const char* SERVICE_STARTED = "service.started";
static constexpr const char* SERVICE_STOPPED = "service.stopped";
static constexpr const char* SERVICE_FAILED = "service.failed";
} // namespace Events

void ServiceRegistry::addService(std::shared_ptr<IService> service) {
	if (!service) return;

	const auto& id = service->getServiceId();
	if (m_serviceMap.count(id)) {
		Log::warn(TAG, "Service '%s' already registered, ignoring", id.c_str());
		return;
	}

	Log::info(TAG, "Registered service: %s (%s)", service->getManifest().serviceName.c_str(), id.c_str());

	m_serviceMap[id] = service;
	m_services.push_back(service);
}

// ──────── Topological Sort (Kahn's Algorithm) ────────

std::vector<std::string> ServiceRegistry::topologicalSort() const {
	// Build adjacency list and in-degree map
	std::unordered_map<std::string, int> inDegree;
	std::unordered_map<std::string, std::vector<std::string>> adj; // dep -> dependents

	// Initialize all nodes
	for (const auto& svc: m_services) {
		const auto& id = svc->getServiceId();
		if (inDegree.find(id) == inDegree.end()) {
			inDegree[id] = 0;
		}
	}

	// Build edges: for each dependency, add edge dep -> service
	for (const auto& svc: m_services) {
		const auto& manifest = svc->getManifest();
		for (const auto& dep: manifest.dependencies) {
			// Only count dependencies on registered services
			if (m_serviceMap.count(dep)) {
				adj[dep].push_back(manifest.serviceId);
				inDegree[manifest.serviceId]++;
			} else {
				Log::warn(TAG, "Service '%s' depends on unregistered '%s' (ignoring)", manifest.serviceId.c_str(), dep.c_str());
			}
		}
	}

	// Kahn's algorithm with priority queue
	// Use a priority queue so services with lower priority value start first
	struct PrioEntry {
		int priority;
		std::string id;
		bool operator>(const PrioEntry& o) const { return priority > o.priority; }
	};
	std::priority_queue<PrioEntry, std::vector<PrioEntry>, std::greater<PrioEntry>> pq;

	for (const auto& [id, degree]: inDegree) {
		if (degree == 0) {
			int prio = m_serviceMap.at(id)->getManifest().priority;
			pq.push({prio, id});
		}
	}

	std::vector<std::string> result;
	while (!pq.empty()) {
		auto [prio, current] = pq.top();
		pq.pop();
		result.push_back(current);

		if (adj.count(current)) {
			for (const auto& neighbor: adj[current]) {
				inDegree[neighbor]--;
				if (inDegree[neighbor] == 0) {
					int np = m_serviceMap.at(neighbor)->getManifest().priority;
					pq.push({np, neighbor});
				}
			}
		}
	}

	if (result.size() != m_services.size()) {
		Log::error(TAG, "Dependency cycle detected! Resolved %zu of %zu services", result.size(), m_services.size());
		// Still return what we could resolve
	}

	return result;
}

// ──────── Lifecycle ────────

bool ServiceRegistry::startAll(bool guiMode) {
	Log::info(TAG, "Starting all services (%zu registered, guiMode=%s)...", m_services.size(), guiMode ? "true" : "false");

	m_bootOrder = topologicalSort();
	m_requiredFailed = false;

	Log::info(TAG, "Boot order resolved:");
	for (size_t i = 0; i < m_bootOrder.size(); i++) {
		auto svc = m_serviceMap[m_bootOrder[i]];
		Log::info(TAG, "  [%zu] %s (priority=%d, required=%s, gui=%s)", i, m_bootOrder[i].c_str(), svc->getManifest().priority, svc->getManifest().required ? "yes" : "no", svc->getManifest().guiRequired ? "yes" : "no");
	}

	int started = 0;
	int skipped = 0;
	int failed = 0;

	for (const auto& id: m_bootOrder) {
		auto it = m_serviceMap.find(id);
		if (it == m_serviceMap.end()) continue;

		auto& svc = it->second;
		const auto& manifest = svc->getManifest();

		// Skip non-autostart services
		if (!manifest.autoStart) {
			Log::info(TAG, "Skipping '%s' (autoStart=false)", id.c_str());
			skipped++;
			continue;
		}

		// Skip GUI-required services if not in GUI mode
		if (manifest.guiRequired && !guiMode) {
			Log::info(TAG, "Skipping '%s' (guiRequired, headless mode)", id.c_str());
			skipped++;
			continue;
		}

		Log::info(TAG, "Starting service: %s...", manifest.serviceName.c_str());

		if (svc->start()) {
			Log::info(TAG, "  ✓ %s started", manifest.serviceName.c_str());
			publishServiceEvent(Events::SERVICE_STARTED, id);
			started++;
		} else {
			Log::error(TAG, "  ✗ %s FAILED to start", manifest.serviceName.c_str());
			publishServiceEvent(Events::SERVICE_FAILED, id);
			failed++;

			if (manifest.required) {
				Log::error(TAG, "CRITICAL: Required service '%s' failed — triggering safe mode", id.c_str());
				m_requiredFailed = true;
			}
		}
	}

	Log::info(TAG, "Service startup complete: %d started, %d skipped, %d failed", started, skipped, failed);

	return !m_requiredFailed;
}

void ServiceRegistry::initGuiServices() {
	Log::info(TAG, "Initializing GUI bridges for started services...");

	for (const auto& id: m_bootOrder) {
		auto it = m_serviceMap.find(id);
		if (it == m_serviceMap.end()) continue;

		auto& svc = it->second;
		if (svc->isRunning()) {
			svc->onGuiInit();
		}
	}

	Log::info(TAG, "GUI bridge initialization complete");
}

void ServiceRegistry::stopAll() {
	Log::info(TAG, "Stopping all services in reverse order...");

	// Stop in reverse boot order
	for (auto it = m_bootOrder.rbegin(); it != m_bootOrder.rend(); ++it) {
		auto sit = m_serviceMap.find(*it);
		if (sit == m_serviceMap.end()) continue;

		auto& svc = sit->second;
		if (svc->isRunning()) {
			Log::info(TAG, "Stopping: %s", svc->getManifest().serviceName.c_str());
			svc->stop();
			publishServiceEvent(Events::SERVICE_STOPPED, *it);
		}
	}

	Log::info(TAG, "All services stopped");
}

bool ServiceRegistry::startService(const std::string& serviceId) {
	auto it = m_serviceMap.find(serviceId);
	if (it == m_serviceMap.end()) {
		Log::error(TAG, "Cannot start unknown service: %s", serviceId.c_str());
		return false;
	}

	auto& svc = it->second;
	if (svc->isRunning()) return true;

	// Ensure dependencies are started first
	for (const auto& depId: svc->getManifest().dependencies) {
		if (!startService(depId)) {
			Log::error(TAG, "Cannot start '%s': dependency '%s' failed", serviceId.c_str(), depId.c_str());
			return false;
		}
	}

	if (svc->start()) {
		publishServiceEvent(Events::SERVICE_STARTED, serviceId);
		return true;
	}

	publishServiceEvent(Events::SERVICE_FAILED, serviceId);
	return false;
}

bool ServiceRegistry::stopService(const std::string& serviceId) {
	auto it = m_serviceMap.find(serviceId);
	if (it == m_serviceMap.end()) return false;

	if (!it->second->isRunning()) return true;

	// Stop dependents first
	auto dependents = findDependents(serviceId);
	for (const auto& depId: dependents) {
		stopService(depId);
	}

	it->second->stop();
	publishServiceEvent(Events::SERVICE_STOPPED, serviceId);
	return true;
}

bool ServiceRegistry::restartService(const std::string& serviceId) {
	Log::info(TAG, "Hot-reloading service: %s", serviceId.c_str());
	stopService(serviceId);
	return startService(serviceId);
}

// ──────── Queries ────────

std::shared_ptr<IService> ServiceRegistry::getService(const std::string& serviceId) const {
	auto it = m_serviceMap.find(serviceId);
	return (it != m_serviceMap.end()) ? it->second : nullptr;
}

ServiceState ServiceRegistry::getServiceState(const std::string& serviceId) const {
	auto it = m_serviceMap.find(serviceId);
	return (it != m_serviceMap.end()) ? it->second->getState() : ServiceState::Stopped;
}

std::vector<std::string> ServiceRegistry::findDependents(const std::string& serviceId) const {
	std::vector<std::string> result;
	for (const auto& svc: m_services) {
		const auto& deps = svc->getManifest().dependencies;
		if (std::find(deps.begin(), deps.end(), serviceId) != deps.end()) {
			result.push_back(svc->getServiceId());
		}
	}
	return result;
}

// ──────── Diagnostics ────────

void ServiceRegistry::performHealthCheck() {
	for (auto& svc: m_services) {
		if (svc->isRunning()) {
			svc->onHealthCheck();
		}
	}
}

void ServiceRegistry::dumpServiceStates() const {
	Log::info(TAG, "=== Service States (%zu services) ===", m_services.size());
	for (const auto& svc: m_services) {
		const auto& m = svc->getManifest();
		auto stats = svc->getServiceStats();
		Log::info(TAG, "  [%s] %s — %s (v%s, starts: %lu, boot: %lld ms, heap: %ld B, required: %s)", serviceStateToString(svc->getState()), m.serviceName.c_str(), m.serviceId.c_str(), m.version.c_str(), (unsigned long)stats.startCount, (long long)(stats.lastStartTimeUs / 1000), (long)stats.heapDeltaBytes, m.required ? "yes" : "no");
	}
	Log::info(TAG, "====================================");
}

void ServiceRegistry::publishServiceEvent(const char* event, const std::string& serviceId) {
	if (m_eventCallback) {
		m_eventCallback(event, serviceId);
	}
}

} // namespace flx::services
