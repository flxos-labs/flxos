#pragma once

#include "lvgl.h"
#include <functional>
#include <string>

namespace System::Apps::Tools {

class Screenshot {
public:

	Screenshot() = default;
	~Screenshot() = default;

	Screenshot(const Screenshot&) = delete;
	Screenshot& operator=(const Screenshot&) = delete;
	Screenshot(Screenshot&&) = delete;
	Screenshot& operator=(Screenshot&&) = delete;

	void createView(lv_obj_t* parent, std::function<void()> onBack);
	lv_obj_t* getView() const { return m_view; }
	void show();
	void hide();
	void destroy();

private:

	std::function<void()> m_onBack {};
	lv_obj_t* m_view {nullptr};
	lv_obj_t* m_statusLabel {nullptr};
	lv_obj_t* m_delaySlider {nullptr};
	lv_obj_t* m_delayValueLabel {nullptr};
	lv_obj_t* m_pathDropdown {nullptr};
	lv_obj_t* m_captureBtn {nullptr};

	void startCapture();
	std::string getSelectedBasePath();

	void updateStatus(const char* msg, bool isError = false);
};

} // namespace System::Apps::Tools
