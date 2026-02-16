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
		int width = 0;
		int height = 0;
		std::string driver;
		std::string bus;
		float sizeInches = 0.0f;
	} display;

	struct Touch {
		bool enabled = false;
		std::string driver;
		std::string bus;
		bool separateBus = false;
	} touch;

	struct SdCard {
		bool supported = false;
		std::string mode;
	} sdCard;

	struct Connectivity {
		bool wifi = false;
		bool bluetooth = false;
		bool bleOnly = false;
		size_t flashSizeKb = 0;
		size_t psramSizeKb = 0;
	} connectivity;
};

} // namespace flx::system
