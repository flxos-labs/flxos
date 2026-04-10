#include <algorithm>
#include <flx/core/BootTimeline.hpp>
#include <flx/core/EventBus.hpp>
#include <flx/core/Logger.hpp>
#include <flx/services/ServiceRegistry.hpp>
#include <queue>
#include <unordered_set>
#include <utility>

#include "esp_timer.h"

static constexpr const char* TAG = "ServiceRegistry";

namespace flx::services {

// Event names for service lifecycle
namespace Events {
static constexpr const char* SERVICE_STARTED = "service.started";
static constexpr const char* SERVICE_STOPPED = "service.stopped";
static constexpr const char* SERVICE_FAILED = "service.failed";
static constexpr const char* SERVICE_RESTARTED = "service.restarted";
static constexpr const char* SERVICE_HEALTH_DEGRADED = "service.health.degraded";
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

	auto allDeps = service->getManifest().allDependencyIds();
	std::vector<std::string> unresolved {};
	for (const auto& dep: allDeps) {
		if (!m_serviceMap.count(dep)) {
			Log::warn(TAG, "Service '%s' depends on '%s' (not yet registered)", id.c_str(), dep.c_str());
			unresolved.push_back(dep);
		}
	}

	if (unresolved.empty()) {
		m_pendingDeps.erase(id);
	} else {
		m_pendingDeps[id] = std::move(unresolved);
	}

	for (auto it = m_pendingDeps.begin(); it != m_pendingDeps.end();) {
		auto& deps = it->second;
		deps.erase(std::remove(deps.begin(), deps.end(), id), deps.end());
		if (deps.empty()) {
			it = m_pendingDeps.erase(it);
		} else {
			++it;
		}
	}

	if (hasCyclicDependencies()) {
		Log::error(TAG, "Dependency cycle detected after registering '%s'", id.c_str());
	}
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
		auto allDeps = manifest.allDependencyIds();
		for (const auto& dep: allDeps) {
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

	auto unresolvedDeps = getUnresolvedDependencies();
	if (!unresolvedDeps.empty()) {
		Log::warn(TAG, "Unresolved service dependencies detected before startup:");
		for (const auto& dep: unresolvedDeps) {
			Log::warn(TAG, "  %s", dep.c_str());
		}
	}
	if (hasCyclicDependencies()) {
		Log::error(TAG, "Registered service graph contains a dependency cycle");
	}

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

	// Pre-calculate which services MUST start due to active profile or being a transitive dependency
	std::unordered_set<std::string> willStart;
	for (auto it = m_bootOrder.rbegin(); it != m_bootOrder.rend(); ++it) {
		auto sit = m_serviceMap.find(*it);
		if (sit == m_serviceMap.end()) continue;
		const auto& manifest = sit->second->getManifest();

		bool explicitStart = true;
		if (!manifest.autoStart) explicitStart = false;
		if (manifest.guiRequired && !guiMode) explicitStart = false;
		if (!isInBootProfile(manifest)) explicitStart = false;

		if (explicitStart) {
			willStart.insert(*it);
		}

		if (willStart.count(*it)) {
			// If this service is starting, its dependencies MUST also start
			for (const auto& dep: manifest.allDependencyIds()) {
				auto depIt = m_serviceMap.find(dep);
				if (depIt != m_serviceMap.end() && depIt->second->getManifest().guiRequired && !guiMode) {
					Log::error(TAG, "Service '%s' depends on guiRequired service '%s', but running in headless mode — skipping dependency",
						it->c_str(), dep.c_str());
					continue;
				}
				willStart.insert(dep);
			}
		}
	}

	for (const auto& id: m_bootOrder) {
		auto it = m_serviceMap.find(id);
		if (it == m_serviceMap.end()) continue;

		auto& svc = it->second;
		const auto& manifest = svc->getManifest();

		if (!willStart.count(id)) {
			if (!manifest.autoStart) {
				Log::info(TAG, "Skipping '%s' (autoStart=false)", id.c_str());
			} else if (manifest.guiRequired && !guiMode) {
				Log::info(TAG, "Skipping '%s' (guiRequired, headless mode)", id.c_str());
			} else if (!isInBootProfile(manifest)) {
				Log::info(TAG, "Skipping '%s' (not in active boot profile)", id.c_str());
			} else {
				Log::info(TAG, "Skipping '%s'", id.c_str());
			}
			skipped++;
			continue;
		}

		Log::info(TAG, "Starting service: %s...", manifest.serviceName.c_str());

		flx::core::BootTimeline::getInstance().record("service:" + id, "start");

		// Ensure dependencies actually started
		auto allDeps = manifest.allDependencyIds();
		bool depsStarted = true;
		for (const auto& dep: allDeps) {
			auto depIt = m_serviceMap.find(dep);
			if (depIt == m_serviceMap.end() || !depIt->second->isRunning()) {
				Log::error(TAG, "  ✗ %s FAILED to start (dependency '%s' not running)", manifest.serviceName.c_str(), dep.c_str());
				depsStarted = false;
				break;
			}
		}

		// Validate API version compatibility for typed dependencies (3.3)
		if (depsStarted) {
			for (const auto& td: manifest.typedDependencies) {
				auto depIt = m_serviceMap.find(td.serviceId);
				if (depIt != m_serviceMap.end()) {
					const auto& depVersion = depIt->second->getManifest().apiVersion;
					if (!depVersion.isCompatibleWith(td.requiredVersion)) {
						Log::error(TAG, "  ✗ %s requires %s v%u.%u.x but found v%u.%u.%u (incompatible)",
							manifest.serviceName.c_str(), td.serviceId.c_str(),
							td.requiredVersion.major, td.requiredVersion.minor,
							depVersion.major, depVersion.minor, depVersion.patch);
						depsStarted = false;
						break;
					}
				}
			}
		}

		if (!depsStarted) {
			flx::core::BootTimeline::getInstance().record("service:" + id, "failed");
			publishServiceEvent(Events::SERVICE_FAILED, id);
			failed++;

			if (manifest.required) {
				Log::error(TAG, "CRITICAL: Required service '%s' failed — triggering safe mode", id.c_str());
				m_requiredFailed = true;
			}
			continue;
		}

		if (svc->start()) {
			Log::info(TAG, "  ✓ %s started", manifest.serviceName.c_str());
			flx::core::BootTimeline::getInstance().record(
				"service:" + id, "started",
				svc->getHeapDeltaBytes(), svc->getLastStartTimeUs());
			publishServiceEvent(Events::SERVICE_STARTED, id);
			started++;
		} else {
			Log::error(TAG, "  ✗ %s FAILED to start", manifest.serviceName.c_str());
			flx::core::BootTimeline::getInstance().record("service:" + id, "failed");
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

	const auto& manifest = svc->getManifest();

	// Cycle protection (Issue 5)
	if (m_startingServices.count(serviceId)) {
		Log::error(TAG, "Circular dependency detected while starting '%s'", serviceId.c_str());
		return false;
	}
	m_startingServices.insert(serviceId);

	// Automated RAII-style cleanup for the starting set
	struct StartingScope {
		std::unordered_set<std::string>& set;
		const std::string& id;
		~StartingScope() { set.erase(id); }
	} scope {m_startingServices, serviceId};

	// Validate API version compatibility for typed dependencies BEFORE starting them (Issue 4)
	for (const auto& td: manifest.typedDependencies) {
		auto depIt = m_serviceMap.find(td.serviceId);
		if (depIt != m_serviceMap.end()) {
			const auto& depVersion = depIt->second->getManifest().apiVersion;
			if (!depVersion.isCompatibleWith(td.requiredVersion)) {
				Log::error(TAG, "Cannot start '%s': incompatible API version for '%s' (required v%u.%u.x, found v%u.%u.%u)",
					serviceId.c_str(), td.serviceId.c_str(),
					td.requiredVersion.major, td.requiredVersion.minor,
					depVersion.major, depVersion.minor, depVersion.patch);
				flx::core::BootTimeline::getInstance().record("service:" + serviceId, "failed");
				publishServiceEvent(Events::SERVICE_FAILED, serviceId);
				return false;
			}
		}
	}

	flx::core::BootTimeline::getInstance().record("service:" + serviceId, "start");

	// Ensure dependencies are started first
	for (const auto& depId: manifest.allDependencyIds()) {
		if (!startService(depId)) {
			Log::error(TAG, "Cannot start '%s': dependency '%s' failed", serviceId.c_str(), depId.c_str());
			flx::core::BootTimeline::getInstance().record("service:" + serviceId, "failed");
			publishServiceEvent(Events::SERVICE_FAILED, serviceId);
			return false;
		}
	}

	if (svc->start()) {
		Log::info(TAG, "  ✓ %s started", manifest.serviceName.c_str());
		flx::core::BootTimeline::getInstance().record(
			"service:" + serviceId, "started",
			svc->getHeapDeltaBytes(), svc->getLastStartTimeUs());
		publishServiceEvent(Events::SERVICE_STARTED, serviceId);
		return true;
	}

	flx::core::BootTimeline::getInstance().record("service:" + serviceId, "failed");
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

std::vector<std::string> ServiceRegistry::getUnresolvedDependencies() const {
	std::vector<std::string> result {};
	for (const auto& [serviceId, deps]: m_pendingDeps) {
		for (const auto& dep: deps) {
			result.push_back(serviceId + " -> " + dep);
		}
	}
	std::sort(result.begin(), result.end());
	return result;
}

bool ServiceRegistry::hasCyclicDependencies() const {
	std::unordered_map<std::string, int> inDegree {};
	std::unordered_map<std::string, std::vector<std::string>> adj {};

	for (const auto& svc: m_services) {
		inDegree[svc->getServiceId()] = 0;
	}

	for (const auto& svc: m_services) {
		const auto& manifest = svc->getManifest();
		auto allDeps = manifest.allDependencyIds();
		for (const auto& dep: allDeps) {
			if (!m_serviceMap.count(dep)) {
				continue;
			}
			adj[dep].push_back(manifest.serviceId);
			inDegree[manifest.serviceId]++;
		}
	}

	std::queue<std::string> ready {};
	for (const auto& [id, degree]: inDegree) {
		if (degree == 0) {
			ready.push(id);
		}
	}

	size_t resolvedCount = 0;
	while (!ready.empty()) {
		auto current = ready.front();
		ready.pop();
		++resolvedCount;

		auto it = adj.find(current);
		if (it == adj.end()) {
			continue;
		}
		for (const auto& neighbor: it->second) {
			if (--inDegree[neighbor] == 0) {
				ready.push(neighbor);
			}
		}
	}

	return resolvedCount != m_services.size();
}

std::vector<std::string> ServiceRegistry::findDependents(const std::string& serviceId) const {
	std::vector<std::string> result;
	for (const auto& svc: m_services) {
		auto allDeps = svc->getManifest().allDependencyIds();
		if (std::find(allDeps.begin(), allDeps.end(), serviceId) != allDeps.end()) {
			result.push_back(svc->getServiceId());
		}
	}
	return result;
}

// ──────── Health Check / Watchdog (2.1) ────────

void ServiceRegistry::performHealthCheck() {
	uint32_t now_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000);

	for (auto& svc: m_services) {
		if (!svc->isRunning()) continue;

		const auto& manifest = svc->getManifest();
		const auto& id = manifest.serviceId;

		// Skip services without watchdog enabled
		if (manifest.healthCheckIntervalMs == 0) continue;

		// Throttle: only check if enough time has elapsed
		auto it = m_lastHealthCheckMs.find(id);
		if (it != m_lastHealthCheckMs.end()) {
			uint32_t elapsed = now_ms - it->second;
			if (elapsed < manifest.healthCheckIntervalMs) continue;
		}
		m_lastHealthCheckMs[id] = now_ms;

		HealthStatus status = svc->onHealthCheck();

		switch (status) {
			case HealthStatus::Healthy:
				break;

			case HealthStatus::Degraded:
				Log::warn(TAG, "Service '%s' health: Degraded", id.c_str());
				publishServiceEvent(Events::SERVICE_HEALTH_DEGRADED, id);
				break;

			case HealthStatus::Unhealthy:
				Log::error(TAG, "Service '%s' health: Unhealthy", id.c_str());
				if (manifest.autoRestart) {
					Log::warn(TAG, "Auto-restarting unhealthy service '%s'", id.c_str());
					svc->stop();
					if (svc->start()) {
						publishServiceEvent(Events::SERVICE_RESTARTED, id);
						Log::info(TAG, "Service '%s' restarted successfully", id.c_str());
					} else {
						publishServiceEvent(Events::SERVICE_FAILED, id);
						Log::error(TAG, "Service '%s' failed to restart", id.c_str());
						if (manifest.required) {
							m_requiredFailed = true;
						}
					}
				}
				break;

			case HealthStatus::Critical:
				Log::error(TAG, "Service '%s' health: CRITICAL", id.c_str());
				publishServiceEvent(Events::SERVICE_FAILED, id);
				if (manifest.required) {
					Log::error(TAG, "CRITICAL: Required service '%s' reports critical health — triggering safe mode", id.c_str());
					m_requiredFailed = true;
				}
				break;
		}
	}
}

// ──────── Service Groups / Boot Profiles (2.3) ────────

bool ServiceRegistry::isInGroup(const ServiceManifest& manifest, const std::string& group) {
	return std::find(manifest.groups.begin(), manifest.groups.end(), group) != manifest.groups.end();
}

bool ServiceRegistry::isInBootProfile(const ServiceManifest& manifest) const {
	if (m_bootProfile.empty()) return true; // No profile set = include everything

	for (const auto& group: m_bootProfile) {
		if (isInGroup(manifest, group)) return true;
	}
	return false;
}

void ServiceRegistry::setBootProfile(const std::vector<std::string>& groups) {
	m_bootProfile = groups;
	Log::info(TAG, "Boot profile set with %zu groups", groups.size());
	for (const auto& g: groups) {
		Log::info(TAG, "  group: %s", g.c_str());
	}
}

std::vector<std::string> ServiceRegistry::getGroups() const {
	std::unordered_set<std::string> seen;
	std::vector<std::string> result;

	for (const auto& svc: m_services) {
		for (const auto& g: svc->getManifest().groups) {
			if (seen.insert(g).second) {
				result.push_back(g);
			}
		}
	}

	std::sort(result.begin(), result.end());
	return result;
}

bool ServiceRegistry::startGroup(const std::string& group) {
	Log::info(TAG, "Starting service group: %s", group.c_str());

	// Use boot order if available, otherwise use registration order
	std::vector<std::string> toStart;
	if (!m_bootOrder.empty()) {
		for (const auto& id: m_bootOrder) {
			auto it = m_serviceMap.find(id);
			if (it == m_serviceMap.end()) continue;
			if (isInGroup(it->second->getManifest(), group)) {
				toStart.push_back(id);
			}
		}
	} else {
		for (const auto& svc: m_services) {
			if (isInGroup(svc->getManifest(), group)) {
				toStart.push_back(svc->getServiceId());
			}
		}
	}

	bool all_ok = true;
	for (const auto& id: toStart) {
		if (!startService(id)) {
			all_ok = false;
			auto it = m_serviceMap.find(id);
			if (it != m_serviceMap.end() && it->second->getManifest().required) {
				m_requiredFailed = true;
			}
		}
	}

	Log::info(TAG, "Group '%s' start complete (%s)", group.c_str(), all_ok ? "OK" : "PARTIAL");
	return all_ok;
}

void ServiceRegistry::stopGroup(const std::string& group) {
	Log::info(TAG, "Stopping service group: %s", group.c_str());

	// Collect services in this group, stop in reverse boot order
	std::vector<std::string> toStop;

	const auto& order = m_bootOrder;
	if (!order.empty()) {
		for (auto it = order.rbegin(); it != order.rend(); ++it) {
			auto sit = m_serviceMap.find(*it);
			if (sit == m_serviceMap.end()) continue;
			if (isInGroup(sit->second->getManifest(), group) && sit->second->isRunning()) {
				toStop.push_back(*it);
			}
		}
	} else {
		for (const auto& svc: m_services) {
			if (isInGroup(svc->getManifest(), group) && svc->isRunning()) {
				toStop.push_back(svc->getServiceId());
			}
		}
	}

	for (const auto& id: toStop) {
		stopService(id);
	}

	Log::info(TAG, "Group '%s' stopped", group.c_str());
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
	flx::core::Bundle data;
	data.putString("serviceId", serviceId);
	flx::core::EventBus::getInstance().publish(event, data);
}

} // namespace flx::services
