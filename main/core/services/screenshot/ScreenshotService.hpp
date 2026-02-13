#pragma once

#include "core/common/Singleton.hpp"
#include "core/services/IService.hpp"
#include "core/services/ServiceManifest.hpp"
#include <string>

namespace System::Services {

/**
 * @brief Service for capturing screenshots as PNG files.
 *
 * Uses LVGL snapshot API with RGB888 color format and lodepng for PNG encoding.
 * Can be called programmatically from any app or tool.
 */
class ScreenshotService : public IService, public Singleton<ScreenshotService> {
	friend class Singleton<ScreenshotService>;

public:

	// ──── IService ────

	static const ServiceManifest serviceManifest;
	const ServiceManifest& getManifest() const override { return serviceManifest; }

	bool onStart() override;
	void onStop() override;

	// ──── Screenshot API ────

	/**
	 * Capture the active screen and save as PNG.
	 * @param savePath  Full VFS path including filename (e.g. "/data/screenshots/scr_001.png")
	 * @return true if capture and save succeeded
	 */
	bool capture(const std::string& savePath);

	/**
	 * Generate a timestamped filename in the given directory.
	 * Creates the directory if needed.
	 * @param basePath  VFS directory (e.g. "/data" or SD mount point)
	 * @return Full path like "/data/screenshots/scr_20260213_185700.png"
	 */
	std::string generateFilename(const std::string& basePath);

private:

	ScreenshotService() = default;
	~ScreenshotService() = default;
};

} // namespace System::Services
