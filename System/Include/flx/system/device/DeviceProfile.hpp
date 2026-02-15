#pragma once

#include <string>
#include <vector>

namespace flx::system {

struct DeviceProfile {
	std::string profileId;
	std::string vendor;
	std::string boardName;
	std::string chipTarget;
	std::string description;

	struct Display {
		int width;
		int height;
		std::string driver;
		std::string bus;
		float sizeInches;
	} display;

	struct Touch {
		bool enabled;
		std::string driver;
		std::string bus;
		bool separateBus;
	} touch;

	struct SdCard {
		bool supported;
		std::string mode;
	} sdCard;

	struct Connectivity {
		bool wifi;
		bool bluetooth;
		bool bleOnly;
		size_t flashSizeKb;
		size_t psramSizeKb;
	} connectivity;
};

} // namespace flx::system
