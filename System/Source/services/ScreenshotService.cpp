#include <Config.hpp>
#include <draw/snapshot/lv_snapshot.h>
#include <flx/core/EventBus.hpp>
#include <flx/core/GuiLock.hpp>
#include <flx/core/Logger.hpp>
#include <flx/system/managers/NotificationManager.hpp>
#include <flx/system/services/ScreenshotService.hpp>
#include <sdkconfig.h>
#if FLXOS_SD_CARD_ENABLED
#include <flx/system/services/SdCardService.hpp>
#endif
// Forward-declare only the C functions we need — including lodepng.h directly
// causes C++ overload conflicts when compiled in a C++ translation unit.
extern "C" {
unsigned lodepng_encode24_file(const char* filename, const unsigned char* image, unsigned w, unsigned h);
const char* lodepng_error_text(unsigned code);
}
#include <lvgl.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string_view>
#include <sys/stat.h>
#include <utility>

static constexpr std::string_view TAG = "ScreenshotService";

namespace flx::services {

namespace {

/**
 * Create a directory directly using POSIX mkdir.
 *
 * This MUST be used instead of going through FileSystemService when the caller
 * already holds the SPI bus lock (e.g. from an LVGL timer callback inside
 * lv_timer_handler()).  The FileSystemService executor runs on a separate task
 * which would need to acquire the same bus lock → deadlock / timeout.
 */
bool ensureDir(const std::string& path) {
	int rc = ::mkdir(path.c_str(), 0777);
	if (rc == 0) return true;
	if (errno == EEXIST) {
		struct stat st {};
		if (::stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
			return true;
		}
	}
	return false;
}

uint8_t blendChannel(uint8_t src, uint8_t srcAlpha, uint8_t dst, uint8_t dstAlpha, uint8_t outAlpha) {
	if (outAlpha == 0) return 0;

	uint32_t srcPremul = static_cast<uint32_t>(src) * srcAlpha;
	uint32_t dstPremul = (static_cast<uint32_t>(dst) * dstAlpha * (255 - srcAlpha) + 127) / 255;
	return static_cast<uint8_t>((srcPremul + dstPremul + (outAlpha / 2)) / outAlpha);
}

void blendOverlayIntoBase(lv_draw_buf_t* base, const lv_draw_buf_t* overlay) {
	if (!base || !overlay || !base->data || !overlay->data) return;
	if (base->header.cf != LV_COLOR_FORMAT_ARGB8888 || overlay->header.cf != LV_COLOR_FORMAT_ARGB8888) return;

	uint32_t blendWidth = LV_MIN(base->header.w, overlay->header.w);
	uint32_t blendHeight = LV_MIN(base->header.h, overlay->header.h);

	for (uint32_t y = 0; y < blendHeight; ++y) {
		auto* dstRow = reinterpret_cast<lv_color32_t*>(static_cast<uint8_t*>(base->data) + y * base->header.stride);
		auto* srcRow = reinterpret_cast<const lv_color32_t*>(static_cast<const uint8_t*>(overlay->data) + y * overlay->header.stride);

		for (uint32_t x = 0; x < blendWidth; ++x) {
			lv_color32_t& dst = dstRow[x];
			const lv_color32_t& src = srcRow[x];

			if (src.alpha == 0) continue;
			if (src.alpha == 255) {
				dst = src;
				continue;
			}

			uint8_t outAlpha = static_cast<uint8_t>(src.alpha + ((static_cast<uint32_t>(dst.alpha) * (255 - src.alpha) + 127) / 255));
			dst.red = blendChannel(src.red, src.alpha, dst.red, dst.alpha, outAlpha);
			dst.green = blendChannel(src.green, src.alpha, dst.green, dst.alpha, outAlpha);
			dst.blue = blendChannel(src.blue, src.alpha, dst.blue, dst.alpha, outAlpha);
			dst.alpha = outAlpha;
		}
	}
}

lv_draw_buf_t* takeSnapshot(lv_obj_t* obj, const char* label) {
	lv_draw_buf_t* snapshot = lv_snapshot_take(obj, LV_COLOR_FORMAT_ARGB8888);
	if (!snapshot || !snapshot->data) {
		Log::warn(TAG, "lv_snapshot_take(ARGB8888) failed for %s", label);
	}
	return snapshot;
}

} // namespace

const ServiceManifest ScreenshotService::serviceManifest = {
	.serviceId = "com.flxos.screenshot",
	.serviceName = "Screenshot Service",
	.version = "0.1.0",
	.dependencies = {},
	.priority = 200,
	.required = false,
	.autoStart = true,
	.guiRequired = true,
	.capabilities = ServiceCapability::Display,
	.description = "RGB888 PNG screenshot capture"};

bool ScreenshotService::onStart() {
	Log::info(TAG, "Screenshot service started");
	return true;
}

void ScreenshotService::onStop() {
	cancelCapture();
	Log::info(TAG, "Screenshot service stopped");
}

bool ScreenshotService::capture(const std::string& savePath, bool notify) {
	flx::core::GuiLock::lock();

	lv_obj_t* screen = lv_screen_active();
	int width = lv_obj_get_width(screen);
	int height = lv_obj_get_height(screen);
	lv_draw_buf_t* snap = takeSnapshot(screen, "active screen");

	if (!snap || !snap->data) {
		Log::error(TAG, "Failed to capture active screen snapshot");
		flx::core::GuiLock::unlock();
		return false;
	}

	/* Virtual keyboard, modals and other overlays live on LVGL's extra layers,
	 * so compose them in explicitly before encoding. */
	lv_draw_buf_t* topLayer = takeSnapshot(lv_layer_top(), "top layer");
	lv_draw_buf_t* sysLayer = takeSnapshot(lv_layer_sys(), "sys layer");
	blendOverlayIntoBase(snap, topLayer);
	blendOverlayIntoBase(snap, sysLayer);

	size_t rgbSize = static_cast<size_t>(width) * height * 3;
	auto* rgbBuf = static_cast<uint8_t*>(malloc(rgbSize));

	if (!rgbBuf) {
		Log::error(TAG, "Failed to allocate RGB buffer (%u bytes)", (unsigned)rgbSize);
		if (topLayer) lv_draw_buf_destroy(topLayer);
		if (sysLayer) lv_draw_buf_destroy(sysLayer);
		lv_draw_buf_destroy(snap);
		flx::core::GuiLock::unlock();
		return false;
	}

	// Copy BGRA -> RGB, handling stride
	for (int y = 0; y < height; y++) {
		const auto* srcRow = reinterpret_cast<const lv_color32_t*>(static_cast<const uint8_t*>(snap->data) + y * snap->header.stride);
		uint8_t* dstRow = rgbBuf + y * width * 3;
		for (int x = 0; x < width; x++) {
			dstRow[x * 3 + 0] = srcRow[x].red;
			dstRow[x * 3 + 1] = srcRow[x].green;
			dstRow[x * 3 + 2] = srcRow[x].blue;
		}
	}

	if (topLayer) lv_draw_buf_destroy(topLayer);
	if (sysLayer) lv_draw_buf_destroy(sysLayer);
	lv_draw_buf_destroy(snap);

	// Save as PNG via lodepng
	// Note: Hold GuiLock during file write to prevent SPI contention with display
	unsigned error = lodepng_encode24_file(savePath.c_str(), rgbBuf, width, height);
	free(rgbBuf);

	flx::core::GuiLock::unlock();

	if (error) {
		Log::error(TAG, "PNG encode failed (error %u): %s", error, lodepng_error_text(error));
		if (notify) {
			flx::system::NotificationManager::getInstance().addNotification(
				"Screenshot Failed",
				"Could not save image",
				"System",
				LV_SYMBOL_WARNING,
				2 // High priority
			);
		}
		return false;
	}

	Log::info(TAG, "Screenshot saved: %s (%dx%d)", savePath.c_str(), width, height);
	if (notify) {
		flx::system::NotificationManager::getInstance().addNotification(
			"Screenshot Saved",
			savePath,
			"System",
			LV_SYMBOL_IMAGE,
			1,
			true,
			"",
			"com.flxos.imageviewer",
			savePath);
	}
	return true;
}

uint32_t ScreenshotService::getDefaultDelay() const {
	return 3;
}

std::string ScreenshotService::getDefaultStoragePath() const {
#if FLXOS_SD_CARD_ENABLED
	if (SdCardService::getInstance().isMounted()) {
		return SdCardService::getInstance().getMountPoint();
	}
#endif
	return "/data";
}

std::string ScreenshotService::generateFilename(const std::string& basePath) {
	std::string dir = basePath + "/screenshots";
	if (!ensureDir(dir)) {
		Log::error(TAG, "Failed to create directory: %s (%s)", dir.c_str(), strerror(errno));

		// Fallback: if the primary path is on SD card, try internal storage
		if (basePath != "/data") {
			Log::warn(TAG, "Falling back to /data for screenshot storage");
			dir = "/data/screenshots";
			if (!ensureDir(dir)) {
				Log::error(TAG, "Fallback directory also failed: %s (%s)", dir.c_str(), strerror(errno));
				return {};
			}
		} else {
			return {};
		}
	}

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
	flx::core::Bundle data;
	data.putString("text", buf);
	flx::core::EventBus::getInstance().publish("ui.overlay.show", data);

	if (m_timer) {
		lv_timer_reset(m_timer);
		lv_timer_resume(m_timer);
	} else {
		m_timer = lv_timer_create(
			[](lv_timer_t* t) {
				auto* self = static_cast<ScreenshotService*>(lv_timer_get_user_data(t));
				self->onTimerTick();
			},
			1000, this);
	}
}

void ScreenshotService::cancelCapture() {
	if (m_timer) {
		lv_timer_pause(m_timer);
	}
	flx::core::EventBus::getInstance().publish("ui.overlay.clear", flx::core::Bundle());
	m_onComplete = nullptr;
}

void ScreenshotService::onTimerTick() {
	m_countdownRemaining--;

	if (m_countdownRemaining <= 0) {
		// Stop timer
		lv_timer_pause(m_timer);

		// Clear overlay
		flx::core::EventBus::getInstance().publish("ui.overlay.clear", flx::core::Bundle());

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
		flx::core::Bundle data;
		data.putString("text", buf);
		flx::core::EventBus::getInstance().publish("ui.overlay.show", data);
	}
}

} // namespace flx::services
