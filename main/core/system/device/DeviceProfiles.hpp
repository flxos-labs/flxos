#pragma once

#include "DeviceProfile.hpp"
#include <string>
#include <vector>

namespace System {

/**
 * @brief Static registry of known device profiles.
 *
 * Contains built-in profiles for popular ESP32 boards.
 * Profiles are defined at static initialization time.
 */
class DeviceProfiles {
public:

	/**
	 * Find a profile by its unique ID.
	 * @return Pointer to profile, or nullptr if not found.
	 */
	static const DeviceProfile* findById(const std::string& profileId);

	/**
	 * Get all registered profiles.
	 */
	static std::vector<const DeviceProfile*> getAll();

	/**
	 * Get all profile IDs.
	 */
	static std::vector<std::string> getAllIds();

	/**
	 * Get the number of built-in profiles.
	 */
	static size_t count();

private:

	DeviceProfiles() = delete; // Static only
};

} // namespace System
