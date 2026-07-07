#pragma once

#include "lvgl.h"
#include <functional>
#include <string>

namespace System::Apps::Tools {

class ScreenRecorder {
public:

	ScreenRecorder() = default;
	~ScreenRecorder() = default;

	ScreenRecorder(const ScreenRecorder&) = delete;
	ScreenRecorder& operator=(const ScreenRecorder&) = delete;
	ScreenRecorder(ScreenRecorder&&) = delete;
	ScreenRecorder& operator=(ScreenRecorder&&) = delete;

	void createView(lv_obj_t* parent, std::function<void()> onBack);
	lv_obj_t* getView() const { return m_view; }

	void update();
	void show();
	void hide();
	void destroy();

private:

	std::function<void()> m_onBack {};
	lv_obj_t* m_view {nullptr};
	lv_obj_t* m_durationSlider {nullptr};
	lv_obj_t* m_durationValueLabel {nullptr};
	lv_obj_t* m_intervalSlider {nullptr};
	lv_obj_t* m_intervalValueLabel {nullptr};
	lv_obj_t* m_pathDropdown {nullptr};
	lv_obj_t* m_recordBtn {nullptr};
	lv_obj_t* m_recordBtnLabel {nullptr};
	lv_obj_t* m_statusLabel {nullptr};

	enum class StatusTone {
		Neutral,
		Success,
		Error,
	};

	StatusTone m_statusTone {StatusTone::Neutral};
	uint32_t m_lastShownFrames {0};
	bool m_lastShownActive {false};

	void toggleRecording();
	void startRecording();
	void stopRecording();
	std::string getSelectedBasePath();
	uint32_t getSelectedIntervalMs() const;
	void updateControls();
	void updateStatus(const char* msg, bool isError = false, bool isNeutral = false);
	void applyStatusTheme();
};

} // namespace System::Apps::Tools
