#pragma once

#include "sd_protocol_types.h"
#include <string>

namespace System::Services {

class SdCardService {
public:

	static SdCardService& getInstance();

	/**
	 * Mount the SD card. Call during hardware init.
	 * @return true if mounted successfully
	 */
	bool mount();

	/**
	 * Unmount the SD card.
	 */
	void unmount();

	/**
	 * @return true if SD card is currently mounted
	 */
	bool isMounted() const { return m_mounted; }

	/**
	 * @return The VFS mount point (e.g., "/sdcard")
	 */
	const std::string& getMountPoint() const { return m_mountPoint; }

private:

	SdCardService();
	~SdCardService() = default;
	SdCardService(const SdCardService&) = delete;
	SdCardService& operator=(const SdCardService&) = delete;

	bool m_mounted = false;
	bool m_busInitializedHere = false;
	sdmmc_card_t* m_card = nullptr;
	std::string m_mountPoint;
};

} // namespace System::Services
