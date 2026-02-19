#pragma once

#include "DeviceProfile.hpp"
#include <string>
#include <vector>

namespace flx::system {

/**
 * @brief Static registry of known device profiles.
 *
 * Contains built-in profiles for popular ESP32 boards.
 * Profiles are defined at static initialization time.
 */
class DeviceProfiles {
public:

	/**
	 * Get the active device profile.
	 *
	 * Returns the single profile defined at compile-time via Config.hpp.
	 */
	static const DeviceProfile& get();

private:

	DeviceProfiles() = delete; // Static only
};

} // namespace flx::system
