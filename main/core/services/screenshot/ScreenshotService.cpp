#include "ScreenshotService.hpp"
#include "core/common/Logger.hpp"
#include "core/services/filesystem/FileSystemService.hpp"
#include "core/tasks/gui/GuiTask.hpp"
#include "draw/snapshot/lv_snapshot.h"
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
		return false;
	}

	Log::info(TAG, "Screenshot saved: %s (%dx%d)", savePath.c_str(), width, height);
	return true;
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
		for (int i = 1; i <= 99999; i++) {
			snprintf(filename, sizeof(filename), "/scr_%05d.png", i);
			std::string full = dir + filename;
			struct stat st {};
			if (stat(full.c_str(), &st) != 0) {
				break;
			}
		}
	}

	return dir + filename;
}

} // namespace System::Services
