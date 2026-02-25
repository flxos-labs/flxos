#pragma once

#include <cstdint>
#include <flx/hal/IDevice.hpp>
#include <functional>
#include <string_view>

namespace flx::hal::gps {

/**
 * @brief Represents a single parsed geographic coordinate.
 * Matches NMEA sentence structures.
 */
struct GpsPosition {
	double latitude = 0.0;
	double longitude = 0.0;
	float altitude = 0.0f; ///< Mean sea level (meters)
	float speedKnots = 0.0f; ///< Speed over ground
	float hdop = 99.0f; ///< Dilution of precision (lower is better, <2 excellent)
	uint8_t satellitesUsed = 0; ///< Number of satellites locked
	bool valid = false; ///< true if the fix is active and accurate
	uint32_t timestampMs = 0; ///< System uptime when this position was recorded
	uint32_t unixTime = 0; ///< UNIX timestamp from satellites
};

/**
 * @brief Abstract interface for a GNSS/GPS receiver device.
 *
 * FlxOS elevates Tactility's ad-hoc Meshtastic GPS port into a formal HAL device.
 * Key enhancements over Tactility:
 *  - Formal GpsState enum.
 *  - subscribePosition replaces ad-hoc UI polling.
 *  - requestColdStart() exposed to clear ephemeral data.
 *  - getGpsModel() directly exposes the auto-probed model.
 */
class IGpsDevice : public flx::hal::IDevice {
public:

	// ── IDevice ───────────────────────────────────────────────────────────
	Type getType() const override { return Type::Gps; }

	enum class GpsState : uint8_t {
		Off, ///< Powered down or in deep sleep
		Searching, ///< Active, but acquiring satellites (no fix)
		FixAcquired, ///< 3D Fix acquired, reporting valid coordinates
		Error ///< Hardware malfunction or UART disconnect
	};

	// ── State and Data ────────────────────────────────────────────────────
	virtual GpsState getGpsState() const = 0;
	virtual GpsPosition getLastPosition() const = 0;

	// ── Subscriptions ─────────────────────────────────────────────────────
	using PositionCallback = std::function<void(const GpsPosition&)>;

	/**
     * @brief Subscribe to coordinate updates.
     * The callback is usually fired at 1Hz immediately following an RMC/GGA parse.
     */
	virtual int subscribePosition(PositionCallback cb) = 0;
	virtual void unsubscribePosition(int id) = 0;

	// ── Auto-prober introspection ─────────────────────────────────────────
	/**
     * @brief The model detected by the runtime auto-prober.
     * e.g., "UBLOX-M10", "ATGM336H".
     */
	virtual std::string_view getGpsModel() const = 0;

	// ── Advanced control ──────────────────────────────────────────────────
	/** Force the GPS module to drop ephemeris and do a fresh sky search. */
	virtual void requestColdStart() {}
	/** Temporarily sleep the RF frontend to save power. */
	virtual void requestSleep(bool sleep) { (void)sleep; }
};

} // namespace flx::hal::gps
