#pragma once

#include <cstdint>
#include <cstdio>
#include <display/lv_display.h>
#include <flx/core/Singleton.hpp>
#include <flx/services/IService.hpp>
#include <flx/services/ServiceManifest.hpp>
#include <functional>
#include <string>

struct _lv_timer_t;

namespace flx::services {

struct ScreenRecordingStats {
	bool active = false;
	uint32_t frameCount = 0;
	uint32_t failedFrames = 0;
	uint32_t maxFrames = 0;
	uint32_t intervalMs = 0;
	std::string directory;
	std::string recordingPath;
};

/**
 * @brief Captures periodic raw RGB565 frames from LVGL display flushes.
 *
 * The recorder wraps the active display flush callback and maintains a shadow
 * framebuffer from pixels LVGL is already sending to the panel. Timer ticks
 * write that buffer into a single .flxrec stream without PNG encoding.
 */
class ScreenRecorderService : public IService, public flx::Singleton<ScreenRecorderService> {
	friend class flx::Singleton<ScreenRecorderService>;

public:

	static const ServiceManifest serviceManifest;
	const ServiceManifest& getManifest() const override { return serviceManifest; }

	bool onStart() override;
	void onStop() override;

	using RecordingCallback = std::function<void(const ScreenRecordingStats& stats)>;

	bool startRecording(
		const std::string& storagePath,
		uint32_t durationSec,
		uint32_t intervalMs,
		RecordingCallback onComplete = nullptr);
	void stopRecording();

	bool isRecording() const { return m_active; }
	ScreenRecordingStats getStats() const;

	uint32_t getDefaultDurationSec() const { return 10; }
	uint32_t getDefaultIntervalMs() const { return 500; }
	std::string getDefaultStoragePath() const;

private:

	ScreenRecorderService();
	~ScreenRecorderService();

	_lv_timer_t* m_timer {nullptr};
	bool m_active {false};
	uint32_t m_frameCount {0};
	uint32_t m_failedFrames {0};
	uint32_t m_maxFrames {0};
	uint32_t m_intervalMs {0};
	uint16_t m_width {0};
	uint16_t m_height {0};
	uint8_t* m_frameBuffer {nullptr};
	size_t m_frameBufferSize {0};
	std::FILE* m_file {nullptr};
	int64_t m_startedAtUs {0};
	int64_t m_durationUs {0};
	bool m_dirtyValid {false};
	lv_area_t m_dirtyArea {};
	std::string m_directory;
	std::string m_recordingPath;
	RecordingCallback m_onComplete {nullptr};
	lv_display_t* m_display {nullptr};
	lv_display_flush_cb_t m_originalFlushCb {nullptr};
	bool m_flushWrapped {false};

	bool prepareRecordingDirectory(const std::string& basePath);
	bool allocateFrameBuffer();
	bool openRecordingFile();
	bool writeHeader(uint32_t frameCount);
	bool writeFrame();
	void markDirty(const lv_area_t& area);
	void closeRecordingFile();
	bool wrapFlushCallback();
	void unwrapFlushCallback();
	void onDisplayFlush(const lv_area_t* area, const uint8_t* pxMap);
	void onTimerTick();
	void finishRecording(bool notify);

	static void flushCallback(lv_display_t* disp, const lv_area_t* area, uint8_t* pxMap);
};

} // namespace flx::services
