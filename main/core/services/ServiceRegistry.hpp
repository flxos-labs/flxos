#pragma once

#include "IService.hpp"
#include "ServiceManifest.hpp"
#include "core/common/Singleton.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace System::Services {

/**
 * @brief Central registry and lifecycle manager for all FlxOS services.
 *
 * Key features:
 * - Dependency resolution via topological sort (Kahn's algorithm)
 * - Priority-based ordering within dependency levels
 * - Health monitoring
 * - Hot reload (stop + re-start individual services)
 * - Safe mode trigger on required service failure
 * - Event bus integration (publishes service lifecycle events)
 *
 * Boot flow:
 *   1. SystemManager calls registerServices() to add all services
 *   2. SystemManager calls startAll() — topo-sorts and starts in order
 *   3. SystemManager calls initGuiServices() — calls onGuiInit() on started services
 *   4. At shutdown, stopAll() stops in reverse order
 */
class ServiceRegistry : public Singleton<ServiceRegistry> {
	friend class Singleton<ServiceRegistry>;

public:

	// ──────── Registration ────────

	/**
	 * Register a service with the registry.
	 * Must be called before startAll().
	 */
	void addService(std::shared_ptr<IService> service);

	// ──────── Lifecycle ────────

	/**
	 * Start all registered services in dependency order.
	 * Uses topological sort to resolve dependencies.
	 * @param guiMode If false, skip services with guiRequired=true
	 * @return true if all required services started successfully
	 */
	bool startAll(bool guiMode = false);

	/**
	 * Initialize GUI bridges for all started services.
	 * Called after LVGL is initialized.
	 */
	void initGuiServices();

	/**
	 * Stop all running services in reverse dependency order.
	 */
	void stopAll();

	/**
	 * Start a specific service by ID (and any unstarted dependencies).
	 * @return true if the service is now in Started state
	 */
	bool startService(const std::string& serviceId);

	/**
	 * Stop a specific service by ID (and any dependents).
	 * @return true if the service was stopped
	 */
	bool stopService(const std::string& serviceId);

	/**
	 * Hot reload: stop and restart a service.
	 * @return true if restarted successfully
	 */
	bool restartService(const std::string& serviceId);

	// ──────── Queries ────────

	/** Get a service by ID */
	std::shared_ptr<IService> getService(const std::string& serviceId) const;

	/** Get the state of a service */
	ServiceState getServiceState(const std::string& serviceId) const;

	/** Get all registered services */
	const std::vector<std::shared_ptr<IService>>& getAllServices() const { return m_services; }

	/** Get the resolved boot order (valid after startAll) */
	const std::vector<std::string>& getBootOrder() const { return m_bootOrder; }

	/** Get count of registered services */
	size_t getServiceCount() const { return m_services.size(); }

	/** Check if a required service failed (triggers safe mode) */
	bool hasRequiredFailure() const { return m_requiredFailed; }

	// ──────── Diagnostics ────────

	/**
	 * Run health checks on all started services.
	 * Logs state of each service.
	 */
	void performHealthCheck();

	/**
	 * Log the current state of all registered services.
	 */
	void dumpServiceStates() const;

private:

	ServiceRegistry() = default;
	~ServiceRegistry() = default;

	/**
	 * Topological sort using Kahn's algorithm.
	 * Returns ordered list of service IDs, or empty if cycle detected.
	 */
	std::vector<std::string> topologicalSort() const;

	/**
	 * Find services that depend on the given service ID.
	 */
	std::vector<std::string> findDependents(const std::string& serviceId) const;

	/**
	 * Publish a service lifecycle event to the EventBus.
	 */
	void publishServiceEvent(const char* event, const std::string& serviceId);

	std::vector<std::shared_ptr<IService>> m_services;
	std::unordered_map<std::string, std::shared_ptr<IService>> m_serviceMap;
	std::vector<std::string> m_bootOrder;
	bool m_requiredFailed = false;
};

} // namespace System::Services
