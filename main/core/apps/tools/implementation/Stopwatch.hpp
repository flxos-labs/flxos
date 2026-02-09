#pragma once

#include "lvgl.h"
#include <cstdint>
#include <functional>

namespace System::Apps::Tools {

class Stopwatch {
public:

	Stopwatch() = default;
	~Stopwatch() = default;

	void createView(lv_obj_t* parent, std::function<void()> onBack);
	lv_obj_t* getView() const { return m_view; }

	// Called periodically by the app loop
	void update();

	// Lifecycle
	void onPause();
	void onStop();

	void show();
	void hide();
	void destroy();

private:

	lv_obj_t* m_view {nullptr};
	lv_obj_t* m_stopwatchLabel {nullptr};
	lv_obj_t* m_stopwatchStartBtn {nullptr};

	uint32_t m_stopwatchStartTime {0};
	uint32_t m_stopwatchElapsed {0};
	bool m_stopwatchRunning {false};

	void updateStopwatchDisplay();
};

} // namespace System::Apps::Tools
