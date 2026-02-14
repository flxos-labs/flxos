#pragma once

#include "lvgl.h"
#include <flx/core/Singleton.hpp>
#include <flx/services/IService.hpp>
#include <flx/services/ServiceManifest.hpp>
#include <functional>
#include <string>

namespace flx::services {

/**
 * @brief Service for capturing screenshots as PNG files.
 *
 * Uses LVGL snapshot API with RGB888 color format and lodepng for PNG encoding.
 * Can be called programmatically from any app or tool.
 */
class ScreenshotService : public IService, public flx::Singleton<ScreenshotService> {
	friend class flx::Singleton<ScreenshotService>;

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

	/**
	 * Get the default delay in seconds for taking a screenshot.
	 */
	uint32_t getDefaultDelay() const;

	/**
	 * Get the default storage path (SD card if mounted, else /data).
	 */
	std::string getDefaultStoragePath() const;

	using CaptureCallback = std::function<void(bool success, const std::string& path)>;

	/**
	 * Schedule a screenshot capture after a delay.
	 * Shows a countdown overlay in the status bar if delay > 0.
	 * @param delaySec  Delay in seconds (0 = instant capture).
	 * @param storagePath  Base storage path (e.g. "/data" or SD mount point).
	 *                     If empty, uses getDefaultStoragePath().
	 * @param onComplete Optional callback when capture finishes/fails.
	 */
	void scheduleCapture(uint32_t delaySec, const std::string& storagePath = "", CaptureCallback onComplete = nullptr);

	/**
	 * Cancel any pending scheduled capture.
	 */
	void cancelCapture();

private:

	ScreenshotService();
	~ScreenshotService();

	lv_timer_t* m_timer {nullptr};
	int m_countdownRemaining {0};
	std::string m_storagePath;
	CaptureCallback m_onComplete {nullptr};
	void onTimerTick();
};

} // namespace flx::services
