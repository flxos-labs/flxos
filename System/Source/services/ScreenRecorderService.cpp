#include <Config.hpp>
#include <display/lv_display_private.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <flx/core/Logger.hpp>
#include <flx/system/managers/NotificationManager.hpp>
#include <flx/system/services/ScreenRecorderService.hpp>
#include <sdkconfig.h>

#if FLXOS_SD_CARD_ENABLED
#include <flx/system/services/SdCardService.hpp>
#endif

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <lvgl.h>
#include <string_view>
#include <sys/stat.h>

static constexpr std::string_view TAG = "ScreenRecorderService";

namespace flx::services {

namespace {

constexpr uint16_t FLXREC_COLOR_RGB565_SWAPPED = 1;

bool ensureDir(const std::string& path) {
	int rc = ::mkdir(path.c_str(), 0777);
	if (rc == 0) return true;
	if (errno == EEXIST) {
		struct stat st {};
		return ::stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
	}
	return false;
}

bool writeBytes(std::FILE* file, const void* data, size_t size) {
	return std::fwrite(data, 1, size, file) == size;
}

bool writeU16(std::FILE* file, uint16_t value) {
	uint8_t bytes[] = {
		static_cast<uint8_t>(value & 0xFFU),
		static_cast<uint8_t>((value >> 8U) & 0xFFU),
	};
	return writeBytes(file, bytes, sizeof(bytes));
}

bool writeU32(std::FILE* file, uint32_t value) {
	uint8_t bytes[] = {
		static_cast<uint8_t>(value & 0xFFU),
		static_cast<uint8_t>((value >> 8U) & 0xFFU),
		static_cast<uint8_t>((value >> 16U) & 0xFFU),
		static_cast<uint8_t>((value >> 24U) & 0xFFU),
	};
	return writeBytes(file, bytes, sizeof(bytes));
}

} // namespace

const ServiceManifest ScreenRecorderService::serviceManifest = {
	.serviceId = "com.flxos.screenrecorder",
	.serviceName = "Screen Recorder Service",
	.version = "0.1.0",
	.dependencies = {},
	.priority = 205,
	.required = false,
	.autoStart = true,
	.guiRequired = true,
	.capabilities = ServiceCapability::Display | ServiceCapability::Storage,
	.description = "Fast delta RGB565 screen recording"};

ScreenRecorderService::ScreenRecorderService() = default;

ScreenRecorderService::~ScreenRecorderService() {
	stopRecording();
	if (m_timer) {
		lv_timer_delete(m_timer);
		m_timer = nullptr;
	}
	if (m_frameBuffer) {
		heap_caps_free(m_frameBuffer);
		m_frameBuffer = nullptr;
	}
}

bool ScreenRecorderService::onStart() {
	Log::info(TAG, "Screen recorder service started");
	return true;
}

void ScreenRecorderService::onStop() {
	stopRecording();
	Log::info(TAG, "Screen recorder service stopped");
}

std::string ScreenRecorderService::getDefaultStoragePath() const {
#if FLXOS_SD_CARD_ENABLED
	if (SdCardService::getInstance().isMounted()) {
		return SdCardService::getInstance().getMountPoint();
	}
#endif
	return "/data";
}

bool ScreenRecorderService::startRecording(
	const std::string& storagePath,
	uint32_t durationSec,
	uint32_t intervalMs,
	RecordingCallback onComplete) {
	stopRecording();

	m_display = lv_display_get_default();
	if (!m_display) {
		Log::error(TAG, "Cannot start recording without an LVGL display");
		return false;
	}

	m_width = static_cast<uint16_t>(lv_display_get_horizontal_resolution(m_display));
	m_height = static_cast<uint16_t>(lv_display_get_vertical_resolution(m_display));
	if (m_width == 0 || m_height == 0) {
		Log::error(TAG, "Cannot start recording with invalid resolution %ux%u", m_width, m_height);
		return false;
	}

	m_intervalMs = intervalMs < 66 ? 66 : intervalMs;
	if (durationSec == 0) {
		durationSec = getDefaultDurationSec();
	}
	m_durationUs = static_cast<int64_t>(durationSec) * 1000000LL;
	m_maxFrames = (durationSec * 1000U + m_intervalMs - 1U) / m_intervalMs;
	if (m_maxFrames == 0) {
		m_maxFrames = 1;
	}

	m_frameCount = 0;
	m_failedFrames = 0;
	m_dirtyValid = false;
	m_recordingPath.clear();
	m_onComplete = onComplete;

	std::string basePath = storagePath.empty() ? getDefaultStoragePath() : storagePath;
	if (!prepareRecordingDirectory(basePath) || !allocateFrameBuffer() || !openRecordingFile() || !wrapFlushCallback()) {
		Log::error(TAG, "Failed to prepare screen recording");
		unwrapFlushCallback();
		closeRecordingFile();
		m_onComplete = nullptr;
		return false;
	}

	m_active = true;
	m_startedAtUs = esp_timer_get_time();

	if (m_timer) {
		lv_timer_set_period(m_timer, m_intervalMs);
		lv_timer_reset(m_timer);
		lv_timer_resume(m_timer);
	} else {
		m_timer = lv_timer_create(
			[](lv_timer_t* timer) {
				auto* self = static_cast<ScreenRecorderService*>(lv_timer_get_user_data(timer));
				self->onTimerTick();
			},
			m_intervalMs, this);
	}

	lv_obj_invalidate(lv_screen_active());

	Log::info(TAG,
		"Started recording: file=%s %ux%u interval=%lums",
		m_recordingPath.c_str(),
		m_width,
		m_height,
		static_cast<unsigned long>(m_intervalMs));
	flx::system::NotificationManager::getInstance().addNotification(
		"Recording Started",
		m_recordingPath,
		"System",
		LV_SYMBOL_VIDEO,
		1);
	return true;
}

void ScreenRecorderService::stopRecording() {
	if (!m_active) {
		if (m_timer) lv_timer_pause(m_timer);
		return;
	}

	finishRecording(true);
}

ScreenRecordingStats ScreenRecorderService::getStats() const {
	return {
		.active = m_active,
		.frameCount = m_frameCount,
		.failedFrames = m_failedFrames,
		.maxFrames = m_maxFrames,
		.intervalMs = m_intervalMs,
		.directory = m_directory,
		.recordingPath = m_recordingPath,
	};
}

bool ScreenRecorderService::prepareRecordingDirectory(const std::string& basePath) {
	std::string root = basePath + "/screen_recordings";
	if (!ensureDir(root)) {
		Log::warn(TAG, "Failed to create %s (%s)", root.c_str(), strerror(errno));
		if (basePath != "/data") {
			root = "/data/screen_recordings";
			if (!ensureDir(root)) {
				Log::error(TAG, "Fallback recording root failed: %s (%s)", root.c_str(), strerror(errno));
				return false;
			}
		} else {
			return false;
		}
	}

	char dirname[64];
	time_t now = 0;
	time(&now);
	struct tm timeinfo = {};
	localtime_r(&now, &timeinfo);

	if (timeinfo.tm_year > 100) {
		snprintf(dirname,
			sizeof(dirname),
			"/rec_%04d%02d%02d_%02d%02d%02d",
			timeinfo.tm_year + 1900,
			timeinfo.tm_mon + 1,
			timeinfo.tm_mday,
			timeinfo.tm_hour,
			timeinfo.tm_min,
			timeinfo.tm_sec);
		m_directory = root + dirname;
		return ensureDir(m_directory);
	}

	for (int i = 1; i <= 9999; ++i) {
		snprintf(dirname, sizeof(dirname), "/rec_%04d", i);
		m_directory = root + dirname;
		struct stat st {};
		if (stat(m_directory.c_str(), &st) != 0) {
			return ensureDir(m_directory);
		}
	}

	Log::error(TAG, "All recording directory slots exhausted");
	return false;
}

bool ScreenRecorderService::allocateFrameBuffer() {
	size_t const required = static_cast<size_t>(m_width) * static_cast<size_t>(m_height) * 2U;
	if (m_frameBuffer && m_frameBufferSize >= required) {
		std::memset(m_frameBuffer, 0, required);
		m_frameBufferSize = required;
		return true;
	}

	if (m_frameBuffer) {
		heap_caps_free(m_frameBuffer);
		m_frameBuffer = nullptr;
		m_frameBufferSize = 0;
	}

	m_frameBuffer = static_cast<uint8_t*>(heap_caps_malloc(required, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
	if (!m_frameBuffer) {
		m_frameBuffer = static_cast<uint8_t*>(heap_caps_malloc(required, MALLOC_CAP_8BIT));
	}
	if (!m_frameBuffer) {
		Log::error(TAG, "Failed to allocate recorder framebuffer (%lu bytes)", static_cast<unsigned long>(required));
		return false;
	}

	std::memset(m_frameBuffer, 0, required);
	m_frameBufferSize = required;
	return true;
}

bool ScreenRecorderService::openRecordingFile() {
	m_recordingPath = m_directory + "/recording.flxrec";
	m_file = std::fopen(m_recordingPath.c_str(), "wb+");
	if (!m_file) {
		Log::error(TAG, "Failed to open %s (%s)", m_recordingPath.c_str(), strerror(errno));
		return false;
	}
	return writeHeader(0);
}

bool ScreenRecorderService::writeHeader(uint32_t frameCount) {
	if (!m_file) return false;

	if (std::fseek(m_file, 0, SEEK_SET) != 0) {
		return false;
	}

	static constexpr char magic[8] = {'F', 'L', 'X', 'R', 'E', 'C', '1', '\0'};
	bool ok = writeBytes(m_file, magic, sizeof(magic)) &&
		writeU16(m_file, m_width) &&
		writeU16(m_file, m_height) &&
		writeU16(m_file, static_cast<uint16_t>(m_intervalMs)) &&
		writeU16(m_file, FLXREC_COLOR_RGB565_SWAPPED) &&
		writeU32(m_file, static_cast<uint32_t>(m_durationUs / 1000LL)) &&
		writeU32(m_file, frameCount) &&
		writeU32(m_file, 0) &&
		writeU32(m_file, 0);

	if (std::fseek(m_file, 0, SEEK_END) != 0) {
		ok = false;
	}
	return ok;
}

bool ScreenRecorderService::writeFrame() {
	if (!m_file || !m_frameBuffer || !m_dirtyValid) {
		return true;
	}

	lv_area_t area = m_dirtyArea;
	area.x1 = std::max<int32_t>(0, area.x1);
	area.y1 = std::max<int32_t>(0, area.y1);
	area.x2 = std::min<int32_t>(m_width - 1, area.x2);
	area.y2 = std::min<int32_t>(m_height - 1, area.y2);

	uint16_t const w = static_cast<uint16_t>(area.x2 - area.x1 + 1);
	uint16_t const h = static_cast<uint16_t>(area.y2 - area.y1 + 1);
	uint32_t const timestampMs = static_cast<uint32_t>((esp_timer_get_time() - m_startedAtUs) / 1000LL);
	uint32_t const payloadBytes = static_cast<uint32_t>(w) * static_cast<uint32_t>(h) * 2U;

	bool ok = writeU32(m_file, timestampMs) &&
		writeU16(m_file, static_cast<uint16_t>(area.x1)) &&
		writeU16(m_file, static_cast<uint16_t>(area.y1)) &&
		writeU16(m_file, w) &&
		writeU16(m_file, h) &&
		writeU32(m_file, payloadBytes);

	for (uint16_t row = 0; ok && row < h; ++row) {
		size_t const srcOffset = (static_cast<size_t>(area.y1 + row) * m_width + area.x1) * 2U;
		ok = writeBytes(m_file, m_frameBuffer + srcOffset, static_cast<size_t>(w) * 2U);
	}

	if (!ok) {
		++m_failedFrames;
		return false;
	}

	++m_frameCount;
	m_dirtyValid = false;
	return true;
}

void ScreenRecorderService::markDirty(const lv_area_t& area) {
	if (!m_dirtyValid) {
		m_dirtyArea = area;
		m_dirtyValid = true;
		return;
	}

	m_dirtyArea.x1 = std::min(m_dirtyArea.x1, area.x1);
	m_dirtyArea.y1 = std::min(m_dirtyArea.y1, area.y1);
	m_dirtyArea.x2 = std::max(m_dirtyArea.x2, area.x2);
	m_dirtyArea.y2 = std::max(m_dirtyArea.y2, area.y2);
}

void ScreenRecorderService::closeRecordingFile() {
	if (!m_file) return;

	writeHeader(m_frameCount);
	std::fflush(m_file);
	std::fclose(m_file);
	m_file = nullptr;
}

bool ScreenRecorderService::wrapFlushCallback() {
	if (!m_display || m_flushWrapped) {
		return m_flushWrapped;
	}

	m_originalFlushCb = m_display->flush_cb;
	if (!m_originalFlushCb) {
		Log::error(TAG, "Cannot record without an LVGL flush callback");
		return false;
	}

	lv_display_set_flush_cb(m_display, &ScreenRecorderService::flushCallback);
	m_flushWrapped = true;
	return true;
}

void ScreenRecorderService::unwrapFlushCallback() {
	if (!m_flushWrapped || !m_display) {
		m_flushWrapped = false;
		m_originalFlushCb = nullptr;
		m_display = nullptr;
		return;
	}

	if (m_display->flush_cb == &ScreenRecorderService::flushCallback) {
		lv_display_set_flush_cb(m_display, m_originalFlushCb);
	}
	m_flushWrapped = false;
	m_originalFlushCb = nullptr;
	m_display = nullptr;
}

void ScreenRecorderService::onDisplayFlush(const lv_area_t* area, const uint8_t* pxMap) {
	if (!m_active || !area || !pxMap || !m_frameBuffer) {
		return;
	}

	int32_t x1 = std::max<int32_t>(0, area->x1);
	int32_t y1 = std::max<int32_t>(0, area->y1);
	int32_t x2 = std::min<int32_t>(m_width - 1, area->x2);
	int32_t y2 = std::min<int32_t>(m_height - 1, area->y2);
	if (x2 < x1 || y2 < y1) {
		return;
	}

	uint32_t const srcWidth = static_cast<uint32_t>(area->x2 - area->x1 + 1);
	uint32_t const copyWidth = static_cast<uint32_t>(x2 - x1 + 1);
	for (int32_t y = y1; y <= y2; ++y) {
		size_t const srcOffset = (static_cast<size_t>(y - area->y1) * srcWidth + static_cast<size_t>(x1 - area->x1)) * 2U;
		size_t const dstOffset = (static_cast<size_t>(y) * m_width + static_cast<size_t>(x1)) * 2U;
		std::memcpy(m_frameBuffer + dstOffset, pxMap + srcOffset, static_cast<size_t>(copyWidth) * 2U);
	}

	lv_area_t clipped {
		.x1 = x1,
		.y1 = y1,
		.x2 = x2,
		.y2 = y2,
	};
	markDirty(clipped);
}

void ScreenRecorderService::onTimerTick() {
	if (!m_active) {
		if (m_timer) lv_timer_pause(m_timer);
		return;
	}

	writeFrame();

	int64_t const elapsedUs = esp_timer_get_time() - m_startedAtUs;
	if (elapsedUs >= m_durationUs || m_frameCount >= m_maxFrames) {
		finishRecording(true);
	}
}

void ScreenRecorderService::finishRecording(bool notify) {
	if (m_timer) {
		lv_timer_pause(m_timer);
	}

	writeFrame();
	m_active = false;
	unwrapFlushCallback();
	closeRecordingFile();

	auto stats = getStats();
	auto callback = m_onComplete;
	m_onComplete = nullptr;

	Log::info(TAG,
		"Recording stopped: file=%s frames=%lu failed=%lu",
		stats.recordingPath.c_str(),
		static_cast<unsigned long>(stats.frameCount),
		static_cast<unsigned long>(stats.failedFrames));

	if (notify) {
		flx::system::NotificationManager::getInstance().addNotification(
			"Recording Saved",
			stats.recordingPath,
			"System",
			LV_SYMBOL_VIDEO,
			stats.failedFrames == 0 ? 1 : 2,
			true,
			"",
			"com.flxos.files",
			stats.recordingPath);
	}

	if (callback) {
		callback(stats);
	}
}

void ScreenRecorderService::flushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* pxMap) {
	auto& recorder = ScreenRecorderService::getInstance();

	if (recorder.m_originalFlushCb) {
		recorder.m_originalFlushCb(disp, area, pxMap);
	}

	if (disp == recorder.m_display) {
		recorder.onDisplayFlush(area, pxMap);
	}
}

} // namespace flx::services
