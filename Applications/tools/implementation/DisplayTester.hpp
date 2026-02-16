#pragma once

#include "lvgl.h"
#include <functional>

namespace System::Apps::Tools {

class DisplayTester {
public:

	DisplayTester() = default;
	~DisplayTester() = default;

	DisplayTester(const DisplayTester&) = delete;
	DisplayTester& operator=(const DisplayTester&) = delete;
	DisplayTester(DisplayTester&&) = delete;
	DisplayTester& operator=(DisplayTester&&) = delete;

	void createView(lv_obj_t* parent, std::function<void()> onBack);
	lv_obj_t* getView() const { return m_view; }
	void show();
	void hide();
	void destroy();

private:

	std::function<void()> m_onBack {};
	lv_obj_t* m_view {nullptr};
	lv_obj_t* m_rgbDisplay {nullptr};
};

} // namespace System::Apps::Tools
