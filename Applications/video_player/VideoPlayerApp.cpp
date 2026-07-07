#include "VideoPlayerApp.hpp"
#include <flx/apps/AppManager.hpp>
#include <flx/core/Logger.hpp>
#include <flx/ui/common/SettingsCommon.hpp>
#include <flx/ui/theming/theme_engine/ThemeEngine.hpp>
#include <flx/ui/theming/themes/Themes.hpp>
#include <flx/ui/theming/ui_constants/UiConstants.hpp>
#include <flx/ui/theming/layout_constants/LayoutConstants.hpp>
#include <esp_timer.h>
#include <cstring>
#include <algorithm>
#include <cstdlib>
#include <cerrno>

using namespace flx::apps;
using namespace flx::ui::common;

namespace System::Apps {

const flx::apps::AppManifest VideoPlayerApp::manifest = {
	.appId = "com.flxos.videoplayer",
	.appName = "Video Player",
	.appIcon = LV_SYMBOL_VIDEO,
	.appVersionName = "0.1.0",
	.appVersionCode = 1,
	.category = AppCategory::System,
	.flags = AppFlags::Hidden,
	.location = AppLocation::internal(),
	.description = "Play screen recordings and videos",
	.sortPriority = 101,
	.capabilities = AppCapability::Storage,
	.requiredServices = {},
	.supportedMimeTypes = {"video/flxrec"},
	.urlSchemes = {},
	.createApp = []() -> std::shared_ptr<App> { return std::make_shared<VideoPlayerApp>(); }};

static constexpr const char* TAG = "VideoPlayerApp";

static inline uint32_t nowMs() {
	return static_cast<uint32_t>(esp_timer_get_time() / 1000);
}

VideoPlayerApp::~VideoPlayerApp() {
	closeVideo();
}

bool VideoPlayerApp::onStart() {
	Log::info(TAG, "Video Player app started");
	return true;
}

bool VideoPlayerApp::onResume() {
	Log::info(TAG, "Video Player app resumed");
	return true;
}

void VideoPlayerApp::onPause() {
	Log::info(TAG, "Video Player app paused");
	pausePlay();
}

void VideoPlayerApp::onStop() {
	Log::info(TAG, "Video Player app stopped");
	closeVideo();
	m_container = nullptr;
	m_page = nullptr;
	m_header = nullptr;
	m_titleLabel = nullptr;
	m_errorLabel = nullptr;
	m_canvasContainer = nullptr;
	m_canvas = nullptr;
	m_playBtn = nullptr;
	m_playBtnLabel = nullptr;
	m_stopBtn = nullptr;
	m_progressSlider = nullptr;
	m_timeLabel = nullptr;
}

void VideoPlayerApp::update() {
	// Periodic updates are handled by the LVGL timer
}

void VideoPlayerApp::onNewIntent(const flx::apps::Intent& intent) {
	if (intent.data.empty()) return;
	closeVideo();
	m_filePath = intent.data;
	m_fileName = getFileName(m_filePath);

	if (m_titleLabel) {
		lv_label_set_text(m_titleLabel, m_fileName.c_str());
	}

	if (m_errorLabel) {
		lv_obj_del(m_errorLabel);
		m_errorLabel = nullptr;
	}
	if (m_canvas) {
		lv_obj_del(m_canvas);
		m_canvas = nullptr;
	}

	if (loadVideo(m_filePath)) {
		seekToStart();
		updateUIControls();
		updateProgressUI();
	}
}

void VideoPlayerApp::createUI(void* parent) {
	auto* parentObj = static_cast<lv_obj_t*>(parent);
	m_container = create_page_container(parentObj);

	if (m_context) {
		m_filePath = m_context->getData();
	}
	m_fileName = m_filePath.empty() ? "Video Player" : getFileName(m_filePath);

	// Header with back button
	lv_obj_t* backBtn = nullptr;
	m_header = create_header(m_container, m_fileName.c_str(), &backBtn);
	lv_obj_remove_flag(m_header, LV_OBJ_FLAG_SCROLLABLE);
	m_titleLabel = lv_obj_get_child(m_header, 1);
	if (m_titleLabel) {
		lv_obj_set_width(m_titleLabel, 0);
		lv_obj_set_flex_grow(m_titleLabel, 1);
		lv_label_set_long_mode(m_titleLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
	}

	lv_obj_add_event_cb(
		backBtn,
		[](lv_event_t* e) {
			auto* app = static_cast<VideoPlayerApp*>(lv_event_get_user_data(e));
			flx::apps::AppManager::getInstance().stopApp(app->getPackageName());
		},
		LV_EVENT_CLICKED, this);

	// Content container
	lv_obj_t* content = lv_obj_create(m_container);
	lv_obj_set_size(content, lv_pct(100), lv_pct(100));
	lv_obj_set_flex_grow(content, 1);
	lv_obj_set_style_border_width(content, 0, 0);
	lv_obj_set_style_pad_all(content, 0, 0);
	lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_scroll_dir(content, LV_DIR_NONE);

	// Canvas wrapper
	m_canvasContainer = lv_obj_create(content);
	lv_obj_set_size(m_canvasContainer, lv_pct(100), lv_pct(100));
	lv_obj_set_flex_grow(m_canvasContainer, 1);
	lv_obj_set_style_bg_color(m_canvasContainer, lv_color_black(), 0);
	lv_obj_set_style_border_width(m_canvasContainer, 0, 0);
	lv_obj_set_style_pad_all(m_canvasContainer, 0, 0);
	lv_obj_set_flex_flow(m_canvasContainer, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(m_canvasContainer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	// Playback Controls Row
	lv_obj_t* controlsRow = lv_obj_create(content);
	lv_obj_set_size(controlsRow, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_style_border_width(controlsRow, 0, 0);
	lv_obj_set_style_pad_all(controlsRow, lv_dpx(UiConstants::PAD_DEFAULT), 0);
	lv_obj_set_flex_flow(controlsRow, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_gap(controlsRow, lv_dpx(UiConstants::PAD_SMALL), 0);

	// Progress slider row
	lv_obj_t* progressRow = lv_obj_create(controlsRow);
	lv_obj_remove_style_all(progressRow);
	lv_obj_set_size(progressRow, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(progressRow, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(progressRow, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	m_progressSlider = lv_slider_create(progressRow);
	lv_obj_set_flex_grow(m_progressSlider, 1);
	lv_obj_add_state(m_progressSlider, LV_STATE_DISABLED);
	lv_obj_set_style_pad_hor(m_progressSlider, lv_dpx(UiConstants::PAD_SMALL), 0);

	m_timeLabel = lv_label_create(progressRow);
	lv_label_set_text(m_timeLabel, "00:00 / 00:00");
	lv_obj_set_style_pad_left(m_timeLabel, lv_dpx(UiConstants::PAD_SMALL), 0);

	// Buttons row
	lv_obj_t* buttonsRow = lv_obj_create(controlsRow);
	lv_obj_remove_style_all(buttonsRow);
	lv_obj_set_size(buttonsRow, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(buttonsRow, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(buttonsRow, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_gap(buttonsRow, lv_dpx(UiConstants::PAD_DEFAULT), 0);

	m_playBtn = lv_button_create(buttonsRow);
	lv_obj_set_size(m_playBtn, lv_dpx(80), lv_dpx(LayoutConstants::SIZE_TOUCH_TARGET));
	m_playBtnLabel = lv_label_create(m_playBtn);
	lv_label_set_text(m_playBtnLabel, LV_SYMBOL_PLAY);
	lv_obj_center(m_playBtnLabel);

	lv_obj_add_event_cb(m_playBtn, [](lv_event_t* e) {
		auto* app = static_cast<VideoPlayerApp*>(lv_event_get_user_data(e));
		app->togglePlay();
	}, LV_EVENT_CLICKED, this);

	m_stopBtn = lv_button_create(buttonsRow);
	lv_obj_set_size(m_stopBtn, lv_dpx(80), lv_dpx(LayoutConstants::SIZE_TOUCH_TARGET));
	lv_obj_t* stopBtnLabel = lv_label_create(m_stopBtn);
	lv_label_set_text(stopBtnLabel, LV_SYMBOL_STOP);
	lv_obj_center(stopBtnLabel);

	lv_obj_add_event_cb(m_stopBtn, [](lv_event_t* e) {
		auto* app = static_cast<VideoPlayerApp*>(lv_event_get_user_data(e));
		app->restartPlay();
	}, LV_EVENT_CLICKED, this);

	ThemeConfig const theme = Themes::GetConfig(ThemeEngine::get_current_theme());
	lv_obj_set_style_bg_color(m_playBtn, theme.success, 0);
	lv_obj_set_style_text_color(m_playBtnLabel, theme.on_primary, 0);
	lv_obj_set_style_bg_color(m_stopBtn, theme.error, 0);
	lv_obj_set_style_text_color(stopBtnLabel, theme.on_primary, 0);

	if (!m_filePath.empty()) {
		if (loadVideo(m_filePath)) {
			seekToStart();
		} else {
			m_errorLabel = lv_label_create(m_canvasContainer);
			lv_label_set_text(m_errorLabel, "Failed to load video");
			lv_obj_set_style_text_color(m_errorLabel, theme.error, 0);
		}
	} else {
		m_errorLabel = lv_label_create(m_canvasContainer);
		lv_label_set_text(m_errorLabel, "No file specified");
		lv_obj_set_style_text_color(m_errorLabel, theme.text_secondary, 0);
	}

	updateUIControls();
	updateProgressUI();
}

bool VideoPlayerApp::loadVideo(const std::string& path) {
	closeVideo();

	m_file = std::fopen(path.c_str(), "rb");
	if (!m_file) {
		Log::error(TAG, "Failed to open video file %s: %s", path.c_str(), std::strerror(errno));
		return false;
	}

	char magic[8];
	if (std::fread(magic, 1, 8, m_file) != 8) {
		Log::error(TAG, "Failed to read magic header");
		closeVideo();
		return false;
	}

	if (std::memcmp(magic, "FLXREC1\0", 8) != 0) {
		Log::error(TAG, "Invalid magic signature in flxrec file");
		closeVideo();
		return false;
	}

	auto readU16 = [this](uint16_t& val) -> bool {
		uint8_t bytes[2];
		if (std::fread(bytes, 1, 2, this->m_file) != 2) return false;
		val = static_cast<uint16_t>(bytes[0] | (bytes[1] << 8));
		return true;
	};

	auto readU32 = [this](uint32_t& val) -> bool {
		uint8_t bytes[4];
		if (std::fread(bytes, 1, 4, this->m_file) != 4) return false;
		val = static_cast<uint32_t>(bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24));
		return true;
	};

	uint16_t format = 0;
	uint32_t reserved1 = 0, reserved2 = 0;
	if (!readU16(m_videoWidth) ||
		!readU16(m_videoHeight) ||
		!readU16(m_intervalMs) ||
		!readU16(format) ||
		!readU32(m_durationMs) ||
		!readU32(m_totalFrames) ||
		!readU32(reserved1) ||
		!readU32(reserved2)) {
		Log::error(TAG, "Failed to parse header fields");
		closeVideo();
		return false;
	}

	if (format != 1) {
		Log::error(TAG, "Unsupported pixel format in flxrec file: %u", format);
		closeVideo();
		return false;
	}

	Log::info(TAG, "Loaded video: %ux%u, interval=%ums, duration=%lums, frames=%lu",
		m_videoWidth, m_videoHeight, m_intervalMs,
		static_cast<unsigned long>(m_durationMs),
		static_cast<unsigned long>(m_totalFrames));

	m_drawBuf = lv_draw_buf_create(m_videoWidth, m_videoHeight, LV_COLOR_FORMAT_RGB565, LV_STRIDE_AUTO);
	if (!m_drawBuf) {
		Log::error(TAG, "Failed to allocate draw buffer for video (%ux%u)", m_videoWidth, m_videoHeight);
		closeVideo();
		return false;
	}

	m_canvas = lv_canvas_create(m_canvasContainer);
	lv_canvas_set_draw_buf(m_canvas, m_drawBuf);
	lv_obj_center(m_canvas);
	lv_canvas_fill_bg(m_canvas, lv_color_black(), LV_OPA_COVER);

	if (m_progressSlider) {
		lv_slider_set_range(m_progressSlider, 0, m_durationMs);
	}

	return true;
}

void VideoPlayerApp::closeVideo() {
	pausePlay();

	if (m_file) {
		std::fclose(m_file);
		m_file = nullptr;
	}

	if (m_canvas) {
		lv_obj_del(m_canvas);
		m_canvas = nullptr;
	}

	if (m_drawBuf) {
		lv_draw_buf_destroy(m_drawBuf);
		m_drawBuf = nullptr;
	}

	m_videoWidth = 0;
	m_videoHeight = 0;
	m_intervalMs = 0;
	m_durationMs = 0;
	m_totalFrames = 0;
	m_currentFrameIndex = 0;
	m_accumulatedPlayTimeMs = 0;
}

void VideoPlayerApp::seekToStart() {
	if (!m_file) return;

	std::fseek(m_file, 32, SEEK_SET);

	m_currentFrameIndex = 0;
	m_accumulatedPlayTimeMs = 0;
	m_nextFrameTimestampMs = 0;
	m_nextFrameFileOffset = 32;

	if (m_totalFrames > 0) {
		uint8_t bytes[4];
		if (std::fread(bytes, 1, 4, m_file) == 4) {
			m_nextFrameTimestampMs = static_cast<uint32_t>(bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24));
			m_nextFrameFileOffset = 32;
		}
	}

	if (m_canvas) {
		lv_canvas_fill_bg(m_canvas, lv_color_black(), LV_OPA_COVER);
		if (m_totalFrames > 0) {
			readNextFrame();
		}
	}
}

bool VideoPlayerApp::readNextFrame() {
	if (!m_file || !m_canvas || !m_drawBuf) return false;

	std::fseek(m_file, m_nextFrameFileOffset, SEEK_SET);

	uint8_t hdr[16];
	if (std::fread(hdr, 1, 16, m_file) != 16) {
		return false;
	}

	[[maybe_unused]] uint32_t timestampMs = static_cast<uint32_t>(hdr[0] | (hdr[1] << 8) | (hdr[2] << 16) | (hdr[3] << 24));
	uint16_t x = static_cast<uint16_t>(hdr[4] | (hdr[5] << 8));
	uint16_t y = static_cast<uint16_t>(hdr[6] | (hdr[7] << 8));
	uint16_t w = static_cast<uint16_t>(hdr[8] | (hdr[9] << 8));
	uint16_t h = static_cast<uint16_t>(hdr[10] | (hdr[11] << 8));
	[[maybe_unused]] uint32_t payloadBytes = static_cast<uint32_t>(hdr[12] | (hdr[13] << 8) | (hdr[14] << 16) | (hdr[15] << 24));

	uint32_t rowBytes = w * 2U;
	uint8_t* rowBuf = static_cast<uint8_t*>(std::malloc(rowBytes));
	if (!rowBuf) {
		Log::error(TAG, "Failed to allocate row buffer (%lu bytes) for frame decoding", static_cast<unsigned long>(rowBytes));
		return false;
	}

	uint8_t* canvasData = m_drawBuf->data;
	uint32_t canvasStride = m_drawBuf->header.stride;

	bool ok = true;
	for (uint16_t row = 0; ok && row < h; ++row) {
		if (std::fread(rowBuf, 1, rowBytes, m_file) != rowBytes) {
			ok = false;
			break;
		}

		if (y + row < m_videoHeight && x < m_videoWidth) {
			uint32_t copyLen = std::min<uint32_t>(rowBytes, (m_videoWidth - x) * 2U);
			size_t const dstOffset = (static_cast<size_t>(y + row) * canvasStride) + (static_cast<size_t>(x) * 2U);
			std::memcpy(canvasData + dstOffset, rowBuf, copyLen);
		}
	}

	std::free(rowBuf);

	if (!ok) {
		return false;
	}

	lv_obj_invalidate(m_canvas);

	m_currentFrameIndex++;
	m_nextFrameFileOffset = std::ftell(m_file);

	if (m_currentFrameIndex < m_totalFrames) {
		uint8_t tsBytes[4];
		if (std::fread(tsBytes, 1, 4, m_file) == 4) {
			m_nextFrameTimestampMs = static_cast<uint32_t>(tsBytes[0] | (tsBytes[1] << 8) | (tsBytes[2] << 16) | (tsBytes[3] << 24));
		}
	}

	return true;
}

void VideoPlayerApp::onTimerTick() {
	if (!m_isPlaying || !m_file) return;

	uint32_t const elapsedRealMs = nowMs() - m_startedPlayTimeMs;
	uint32_t currentPosMs = m_accumulatedPlayTimeMs + elapsedRealMs;

	while (m_isPlaying && m_currentFrameIndex < m_totalFrames && currentPosMs >= m_nextFrameTimestampMs) {
		if (!readNextFrame()) {
			pausePlay();
			break;
		}
	}

	if (m_currentFrameIndex >= m_totalFrames) {
		pausePlay();
		currentPosMs = m_durationMs;
	}

	updateProgressUI();
}

void VideoPlayerApp::togglePlay() {
	if (m_isPlaying) {
		pausePlay();
	} else {
		startPlay();
	}
}

void VideoPlayerApp::startPlay() {
	if (m_isPlaying || !m_file) return;

	if (m_currentFrameIndex >= m_totalFrames) {
		seekToStart();
	}

	m_isPlaying = true;
	m_startedPlayTimeMs = nowMs();

	if (m_playbackTimer) {
		lv_timer_resume(m_playbackTimer);
	} else {
		m_playbackTimer = lv_timer_create(
			[](lv_timer_t* timer) {
				auto* self = static_cast<VideoPlayerApp*>(lv_timer_get_user_data(timer));
				self->onTimerTick();
			},
			20, this);
	}

	updateUIControls();
}

void VideoPlayerApp::pausePlay() {
	if (!m_isPlaying) return;

	m_isPlaying = false;
	m_accumulatedPlayTimeMs += (nowMs() - m_startedPlayTimeMs);

	if (m_playbackTimer) {
		lv_timer_pause(m_playbackTimer);
	}

	updateUIControls();
}

void VideoPlayerApp::restartPlay() {
	pausePlay();
	seekToStart();
	updateUIControls();
	updateProgressUI();
}

void VideoPlayerApp::updateUIControls() {
	if (m_playBtnLabel) {
		lv_label_set_text(m_playBtnLabel, m_isPlaying ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
		lv_obj_center(m_playBtnLabel);
	}
}

void VideoPlayerApp::updateProgressUI() {
	uint32_t currentPosMs = m_accumulatedPlayTimeMs;
	if (m_isPlaying) {
		currentPosMs += (nowMs() - m_startedPlayTimeMs);
	}
	if (currentPosMs > m_durationMs) {
		currentPosMs = m_durationMs;
	}

	if (m_progressSlider) {
		lv_slider_set_value(m_progressSlider, currentPosMs, LV_ANIM_OFF);
	}

	if (m_timeLabel) {
		std::string timeStr = formatTime(currentPosMs) + " / " + formatTime(m_durationMs);
		lv_label_set_text(m_timeLabel, timeStr.c_str());
	}
}

std::string VideoPlayerApp::formatTime(uint32_t ms) const {
	uint32_t totalSec = ms / 1000;
	uint32_t min = totalSec / 60;
	uint32_t sec = totalSec % 60;
	char buf[16];
	std::snprintf(buf, sizeof(buf), "%02lu:%02lu", static_cast<unsigned long>(min), static_cast<unsigned long>(sec));
	return std::string(buf);
}

std::string VideoPlayerApp::getFileName(const std::string& path) const {
	auto pos = path.rfind('/');
	if (pos != std::string::npos && pos + 1 < path.size()) {
		return path.substr(pos + 1);
	}
	return path;
}

} // namespace System::Apps
