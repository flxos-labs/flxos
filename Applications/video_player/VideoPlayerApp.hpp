#pragma once

#include "lvgl.h"
#include <flx/apps/App.hpp>
#include <flx/apps/AppManifest.hpp>
#include <cstdio>
#include <string>

namespace System::Apps {

class VideoPlayerApp : public flx::apps::App {
public:
	static const flx::apps::AppManifest manifest;

	VideoPlayerApp() = default;
	~VideoPlayerApp() override;

	// === App lifecycle ===
	bool onStart() override;
	bool onResume() override;
	void onPause() override;
	void onStop() override;
	void update() override;
	void createUI(void* parent) override;
	void onNewIntent(const flx::apps::Intent& intent) override;

	std::string getPackageName() const override { return manifest.appId; }
	std::string getAppName() const override { return manifest.appName; }
	const void* getIcon() const override { return manifest.appIcon; }

private:
	std::string m_filePath;
	std::string m_fileName;

	// UI elements
	lv_obj_t* m_container{nullptr};
	lv_obj_t* m_page{nullptr};
	lv_obj_t* m_header{nullptr};
	lv_obj_t* m_titleLabel{nullptr};
	lv_obj_t* m_errorLabel{nullptr};
	lv_obj_t* m_canvasContainer{nullptr};
	lv_obj_t* m_canvas{nullptr};
	lv_obj_t* m_playBtn{nullptr};
	lv_obj_t* m_playBtnLabel{nullptr};
	lv_obj_t* m_stopBtn{nullptr};
	lv_obj_t* m_progressSlider{nullptr};
	lv_obj_t* m_timeLabel{nullptr};

	// LVGL canvas buffer
	lv_draw_buf_t* m_drawBuf{nullptr};

	// Playback State
	std::FILE* m_file{nullptr};
	uint16_t m_videoWidth{0};
	uint16_t m_videoHeight{0};
	uint16_t m_intervalMs{0};
	uint32_t m_durationMs{0};
	uint32_t m_totalFrames{0};

	bool m_isPlaying{false};
	uint32_t m_currentFrameIndex{0};
	uint32_t m_accumulatedPlayTimeMs{0};
	uint32_t m_startedPlayTimeMs{0};
	uint32_t m_nextFrameTimestampMs{0};
	long m_nextFrameFileOffset{0};

	lv_timer_t* m_playbackTimer{nullptr};

	// Helper methods
	bool loadVideo(const std::string& path);
	void closeVideo();
	void togglePlay();
	void startPlay();
	void pausePlay();
	void restartPlay();
	void seekToStart();
	bool readNextFrame();
	void onTimerTick();
	void updateUIControls();
	void updateProgressUI();
	std::string formatTime(uint32_t ms) const;
	std::string getFileName(const std::string& path) const;
};

} // namespace System::Apps
