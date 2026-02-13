#include "ScreenshotService.hpp"
#include "core/common/Logger.hpp"
#include "core/services/filesystem/FileSystemService.hpp"
#include "core/system/notification/NotificationManager.hpp"
#include "core/tasks/gui/GuiTask.hpp"
#include "core/ui/desktop/modules/status_bar/StatusBar.hpp"
#include "draw/snapshot/lv_snapshot.h"
#include "sdkconfig.h"
#if defined(CONFIG_FLXOS_SD_CARD_ENABLED)
#include "core/services/storage/SdCardService.hpp"
#endif
// Forward-declare only the C functions we need — including lodepng.h directly
// causes C++ overload conflicts when compiled in a C++ translation unit.
extern "C" {
unsigned lodepng_encode24_file(const char* filename, const unsigned char* image, unsigned w, unsigned h);
const char* lodepng_error_text(unsigned code);
}
#include "lvgl.h"

#include <cstdio>
#include <cstring>
#include <ctime>
#include <sys/stat.h>

static constexpr std::string_view TAG = "ScreenshotService";

namespace System::Services {

const ServiceManifest ScreenshotService::serviceManifest = {
	.serviceId = "com.flxos.screenshot",
	.serviceName = "Screenshot Service",
	.version = "1.0.0",
	.dependencies = {},
	.priority = 200,
	.required = false,
	.autoStart = true,
	.guiRequired = true,
	.capabilities = ServiceCapability::Display,
	.description = "RGB888 PNG screenshot capture"
};

bool ScreenshotService::onStart() {
	Log::info(TAG, "Screenshot service started");
	return true;
}

void ScreenshotService::onStop() {
	cancelCapture();
	Log::info(TAG, "Screenshot service stopped");
}

bool ScreenshotService::capture(const std::string& savePath) {
	GuiTask::lock();
	lv_obj_t* screen = lv_screen_active();
	int width = lv_obj_get_width(screen);
	int height = lv_obj_get_height(screen);
	lv_draw_buf_t* snap = lv_snapshot_take(screen, LV_COLOR_FORMAT_RGB888);
	GuiTask::unlock();

	if (!snap || !snap->data) {
		Log::error(TAG, "lv_snapshot_take(RGB888) failed");
		return false;
	}

	// LVGL RGB888 stores as B-G-R per pixel, lodepng needs R-G-B
	uint8_t* data = snap->data;
	uint32_t stride = snap->header.stride;
	size_t rgbSize = static_cast<size_t>(width) * height * 3;
	auto* rgbBuf = static_cast<uint8_t*>(malloc(rgbSize));

	if (!rgbBuf) {
		Log::error(TAG, "Failed to allocate RGB buffer (%u bytes)", (unsigned)rgbSize);
		lv_draw_buf_destroy(snap);
		return false;
	}

	// Copy with BGR→RGB swap, handling stride
	for (int y = 0; y < height; y++) {
		const uint8_t* srcRow = data + y * stride;
		uint8_t* dstRow = rgbBuf + y * width * 3;
		for (int x = 0; x < width; x++) {
			dstRow[x * 3 + 0] = srcRow[x * 3 + 2]; // R ← B
			dstRow[x * 3 + 1] = srcRow[x * 3 + 1]; // G ← G
			dstRow[x * 3 + 2] = srcRow[x * 3 + 0]; // B ← R
		}
	}

	lv_draw_buf_destroy(snap);

	// Save as PNG via lodepng
	unsigned error = lodepng_encode24_file(savePath.c_str(), rgbBuf, width, height);
	free(rgbBuf);

	if (error) {
		Log::error(TAG, "PNG encode failed (error %u): %s", error, lodepng_error_text(error));
		System::NotificationManager::getInstance().addNotification(
			"Screenshot Failed",
			"Could not save image",
			"System",
			LV_SYMBOL_WARNING,
			2 // High priority
		);
		return false;
	}

	Log::info(TAG, "Screenshot saved: %s (%dx%d)", savePath.c_str(), width, height);
	System::NotificationManager::getInstance().addNotification(
		"Screenshot Saved",
		savePath,
		"System",
		LV_SYMBOL_IMAGE
	);
	return true;
}

uint32_t ScreenshotService::getDefaultDelay() const {
	return 3;
}

std::string ScreenshotService::getDefaultStoragePath() const {
#if defined(CONFIG_FLXOS_SD_CARD_ENABLED)
	if (SdCardService::getInstance().isMounted()) {
		return SdCardService::getInstance().getMountPoint();
	}
#endif
	return "/data";
}

std::string ScreenshotService::generateFilename(const std::string& basePath) {
	std::string dir = basePath + "/screenshots";
	FileSystemService::getInstance().mkdir(dir);

	char filename[64];
	time_t now = 0;
	time(&now);
	struct tm timeinfo = {};
	localtime_r(&now, &timeinfo);

	if (timeinfo.tm_year > 100) { // RTC is set
		snprintf(filename, sizeof(filename), "/scr_%04d%02d%02d_%02d%02d%02d.png", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
	} else {
		bool found = false;
		for (int i = 1; i <= 99999; i++) {
			snprintf(filename, sizeof(filename), "/scr_%05d.png", i);
			std::string full = dir + filename;
			struct stat st {};
			if (stat(full.c_str(), &st) != 0) {
				found = true;
				break;
			}
		}
		if (!found) {
			Log::error(TAG, "All screenshot filename slots exhausted");
			return {};
		}
	}

	return dir + filename;
}

ScreenshotService::ScreenshotService() = default;

ScreenshotService::~ScreenshotService() {
	if (m_timer) {
		lv_timer_delete(m_timer);
		m_timer = nullptr;
	}
}

void ScreenshotService::scheduleCapture(uint32_t delaySec, const std::string& storagePath, CaptureCallback onComplete) {
	// Cancel any existing pending capture/timer
	cancelCapture();

	m_onComplete = onComplete; // Store new callback
	m_storagePath = storagePath.empty() ? getDefaultStoragePath() : storagePath;

	if (delaySec == 0) {
		// Instant capture
		std::string path = generateFilename(m_storagePath);
		if (path.empty()) {
			Log::error(TAG, "Failed to generate screenshot filename");
			if (m_onComplete) {
				m_onComplete(false, "");
				m_onComplete = nullptr;
			}
			return;
		}
		bool res = capture(path);

		if (m_onComplete) {
			m_onComplete(res, path);
			m_onComplete = nullptr;
		}
		return;
	}

	m_countdownRemaining = static_cast<int>(delaySec);
	if (m_countdownRemaining < 1) m_countdownRemaining = 1;

	// Show initial overlay
	char buf[16];
	snprintf(buf, sizeof(buf), LV_SYMBOL_IMAGE " %d", m_countdownRemaining);
	UI::Modules::StatusBar::showOverlay(buf);

	if (m_timer) {
		lv_timer_reset(m_timer);
		lv_timer_resume(m_timer);
	} else {
		m_timer = lv_timer_create(
			[](lv_timer_t* t) {
				auto* self = static_cast<ScreenshotService*>(lv_timer_get_user_data(t));
				self->onTimerTick();
			},
			1000, this
		);
	}
}

void ScreenshotService::cancelCapture() {
	if (m_timer) {
		lv_timer_pause(m_timer);
	}
	UI::Modules::StatusBar::clearOverlay();
	m_onComplete = nullptr;
}

void ScreenshotService::onTimerTick() {
	m_countdownRemaining--;

	if (m_countdownRemaining <= 0) {
		// Stop timer
		lv_timer_pause(m_timer);

		// Clear overlay
		UI::Modules::StatusBar::clearOverlay();

		// Capture
		std::string path = generateFilename(m_storagePath);
		bool res = false;
		if (path.empty()) {
			Log::error(TAG, "Failed to generate screenshot filename");
		} else {
			res = capture(path);
		}

		if (m_onComplete) {
			m_onComplete(res, path);
			m_onComplete = nullptr;
		}
	} else {
		// Update overlay
		char buf[16];
		snprintf(buf, sizeof(buf), LV_SYMBOL_IMAGE " %d", m_countdownRemaining);
		UI::Modules::StatusBar::showOverlay(buf);
	}
}

} // namespace System::Services
