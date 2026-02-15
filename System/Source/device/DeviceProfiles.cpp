#include <algorithm>
#include <flx/system/device/DeviceProfiles.hpp>

namespace flx::system {

static const DeviceProfile generic_esp32 = {
	.profileId = "generic-esp32",
	.vendor = "Espressif",
	.boardName = "Generic ESP32",
	.chipTarget = "ESP32",
	.description = "Default generic profile",
	.display = {
		.width = 320,
		.height = 240,
		.driver = "ILI9341",
		.bus = "SPI",
		.sizeInches = 2.4f
	},
	.touch = {.enabled = false, .driver = "None", .bus = "None", .separateBus = false},
	.sdCard = {.supported = true, .mode = "SPI"},
	.connectivity = {.wifi = true, .bluetooth = true, .bleOnly = false, .flashSizeKb = 4096, .psramSizeKb = 0}
};

static const std::vector<const DeviceProfile*> profiles = {
	&generic_esp32
};

const DeviceProfile* DeviceProfiles::findById(const std::string& profileId) {
	for (const auto* profile: profiles) {
		if (profile->profileId == profileId) {
			return profile;
		}
	}
	return nullptr;
}

std::vector<const DeviceProfile*> DeviceProfiles::getAll() {
	return profiles;
}

std::vector<std::string> DeviceProfiles::getAllIds() {
	std::vector<std::string> ids;
	for (const auto* profile: profiles) {
		ids.push_back(profile->profileId);
	}
	return ids;
}

size_t DeviceProfiles::count() {
	return profiles.size();
}

} // namespace flx::system
