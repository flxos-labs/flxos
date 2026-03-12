#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace flx::services {

/**
 * @brief Service capability flags (mirrors AppCapability for consistency)
 */
enum class ServiceCapability : uint32_t {
	None = 0,
	WiFi = (1 << 0),
	Bluetooth = (1 << 1),
	Storage = (1 << 2),
	GPIO = (1 << 3),
	I2C = (1 << 4),
	Display = (1 << 5),
	Audio = (1 << 6),
};

inline ServiceCapability operator|(ServiceCapability a, ServiceCapability b) {
	return static_cast<ServiceCapability>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline bool operator&(ServiceCapability a, ServiceCapability b) {
	return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b)) != 0;
}

/**
 * @brief Health status returned by periodic service health checks.
 *
 * Services override onHealthCheck() to return one of these.
 * The ServiceRegistry watchdog uses the result to log, restart,
 * or trigger safe mode.
 */
enum class HealthStatus {
	Healthy, ///< Service is operating normally
	Degraded, ///< Log warning, keep running (e.g. queue backlog)
	Unhealthy, ///< Auto-restart if autoRestart=true in manifest
	Critical, ///< Trigger safe mode if required=true in manifest
};

/**
 * @brief Service lifecycle state machine
 *
 * Stopped → Starting → Started → Stopping → Stopped
 *                    ↘ Failed
 */
enum class ServiceState {
	Stopped,
	Starting,
	Started,
	Stopping,
	Failed,
};

inline const char* serviceStateToString(ServiceState state) {
	switch (state) {
		case ServiceState::Stopped:
			return "Stopped";
		case ServiceState::Starting:
			return "Starting";
		case ServiceState::Started:
			return "Started";
		case ServiceState::Stopping:
			return "Stopping";
		case ServiceState::Failed:
			return "Failed";
		default:
			return "Unknown";
	}
}

/**
 * @brief Semantic API version for service compatibility checking.
 *
 * Follows semver: breaking changes bump major, new features bump minor,
 * bug fixes bump patch. A dependency is compatible if the provider's
 * major version matches and its minor version is >= the required minor.
 */
struct ApiVersion {
	uint8_t major = 1; ///< Breaking changes
	uint8_t minor = 0; ///< New features (backward-compatible)
	uint8_t patch = 0; ///< Bug fixes

	/**
	 * Check if this version satisfies a required version.
	 * Compatible when major matches and minor >= required minor.
	 */
	bool isCompatibleWith(const ApiVersion& required) const {
		return major == required.major && minor >= required.minor;
	}

	bool operator==(const ApiVersion& o) const {
		return major == o.major && minor == o.minor && patch == o.patch;
	}
	bool operator!=(const ApiVersion& o) const { return !(*this == o); }
};

/**
 * @brief Versioned dependency declaration.
 *
 * Pairs a service ID with the minimum API version required.
 * Used alongside the legacy `dependencies` vector for backward compatibility.
 */
struct ServiceDependency {
	std::string serviceId;
	ApiVersion requiredVersion {1, 0, 0};
};

/**
 * @brief Static metadata for a service registration.
 *
 * Mirrors the AppManifest pattern from Phase 1.
 * Each service defines a static manifest that the ServiceRegistry uses
 * for dependency resolution and boot ordering.
 */
struct ServiceManifest {
	/// Unique service identifier (e.g. "com.flxos.settings")
	std::string serviceId;

	/// Human-readable name
	std::string serviceName;

	/// Service version string (human-readable, e.g. "1.2.3")
	std::string version = "1.0.0";

	/// Structured API version for compatibility checking (3.3)
	ApiVersion apiVersion {1, 0, 0};

	/// IDs of services this service depends on (must be started first)
	/// Retained for backward compatibility — simple string-based deps
	std::vector<std::string> dependencies {};

	/// Versioned dependencies for API compatibility validation (3.3)
	/// During topo sort, each typed dependency's actual apiVersion is
	/// checked against the requiredVersion.
	std::vector<ServiceDependency> typedDependencies {};

	/// Boot priority within the same dependency level (lower = earlier)
	int priority = 100;

	/// If true, failure to start triggers safe mode
	bool required = false;

	/// If true, automatically started during boot
	bool autoStart = true;

	/// If true, this service requires GUI mode (skipped in headless)
	bool guiRequired = false;

	/// Capability flags this service provides
	ServiceCapability capabilities = ServiceCapability::None;

	/// Human-readable description
	std::string description {};

	// ─── Watchdog (2.1) ───

	/// Interval in ms for periodic health checks (0 = disabled)
	uint32_t healthCheckIntervalMs = 0;

	/// If true, automatically restart the service when onHealthCheck()
	/// returns HealthStatus::Unhealthy
	bool autoRestart = false;

	// ─── Groups / Boot Profiles (2.3) ───

	/// Groups this service belongs to — used by boot profiles and group start/stop
	std::vector<std::string> groups {"default"};

	// ─── Helpers ───

	/**
	 * Get all dependency service IDs (merges both simple and typed deps).
	 * Useful for topo sort which only needs the IDs.
	 */
	std::vector<std::string> allDependencyIds() const {
		std::vector<std::string> result = dependencies;
		for (const auto& td: typedDependencies) {
			// Avoid duplicates
			bool found = false;
			for (const auto& existing: result) {
				if (existing == td.serviceId) {
					found = true;
					break;
				}
			}
			if (!found) result.push_back(td.serviceId);
		}
		return result;
	}
};

} // namespace flx::services
