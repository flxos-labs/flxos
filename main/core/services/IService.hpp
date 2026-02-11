#pragma once

#include "ServiceManifest.hpp"
#include <string>

namespace System::Services {

/**
 * @brief Abstract interface for all FlxOS services.
 *
 * Every system service (SettingsManager, DisplayManager, ConnectivityManager,
 * etc.) implements this interface to participate in the ServiceRegistry
 * lifecycle: dependency resolution, ordered startup, health monitoring,
 * and hot reload.
 *
 * State machine:
 *   Stopped → Starting → Started → Stopping → Stopped
 *                      ↘ Failed
 *
 * Usage:
 *   1. Define a static ServiceManifest in your service class.
 *   2. Inherit from IService and implement onStart() / onStop().
 *   3. Register with ServiceRegistry::addService() during boot.
 */
class IService {
public:

	virtual ~IService() = default;

	// ──────── Lifecycle hooks (override these) ────────

	/**
	 * Called when the registry starts this service.
	 * Dependencies are guaranteed to be in Started state.
	 * @return true if started successfully, false triggers Failed state.
	 */
	virtual bool onStart() = 0;

	/**
	 * Called when the registry stops this service.
	 * Dependents are guaranteed to be stopped already.
	 */
	virtual void onStop() = 0;

	/**
	 * Optional: called during GUI init phase for services that need
	 * LVGL bridges or other GUI-only initialization.
	 * Only called if the service manifest has guiRequired = false
	 * (i.e., the service starts in headless but has optional GUI bits).
	 */
	virtual void onGuiInit() {}

	/**
	 * Optional health check. Called periodically by the registry.
	 * Override to report service health status.
	 */
	virtual void onHealthCheck() {}

	// ──────── State management (non-virtual) ────────

	/**
	 * Start the service (manages state transitions).
	 * @return true if service reached Started state.
	 */
	bool start() {
		if (m_state == ServiceState::Started || m_state == ServiceState::Starting) {
			return m_state == ServiceState::Started;
		}
		m_state = ServiceState::Starting;
		if (onStart()) {
			m_state = ServiceState::Started;
			m_startCount++;
			return true;
		}
		m_state = ServiceState::Failed;
		return false;
	}

	/**
	 * Stop the service (manages state transitions).
	 */
	void stop() {
		if (m_state != ServiceState::Started) return;
		m_state = ServiceState::Stopping;
		onStop();
		m_state = ServiceState::Stopped;
	}

	/**
	 * Hot reload: stop then start.
	 * @return true if restarted successfully.
	 */
	bool restart() {
		stop();
		return start();
	}

	// ──────── Accessors ────────

	ServiceState getState() const { return m_state; }
	bool isRunning() const { return m_state == ServiceState::Started; }
	uint32_t getStartCount() const { return m_startCount; }

	/// Each service must provide its manifest
	virtual const ServiceManifest& getManifest() const = 0;

	/// Convenience: get service ID from manifest
	const std::string& getServiceId() const { return getManifest().serviceId; }

protected:

	ServiceState m_state = ServiceState::Stopped;
	uint32_t m_startCount = 0;
};

} // namespace System::Services
