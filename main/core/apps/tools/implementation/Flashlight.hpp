#pragma once

#include "lvgl.h"
#include <functional>

namespace System::Apps::Tools {

class Flashlight {
public:

	Flashlight() = default;
	~Flashlight() = default;

	Flashlight(const Flashlight&) = delete;
	Flashlight& operator=(const Flashlight&) = delete;
	Flashlight(Flashlight&&) = delete;
	Flashlight& operator=(Flashlight&&) = delete;

	void createView(lv_obj_t* parent, std::function<void()> onBack);
	lv_obj_t* getView() const { return m_view; }
	void show();
	void hide();
	void destroy();

private:

	std::function<void()> m_onBack {};
	lv_obj_t* m_view {nullptr};
	lv_obj_t* m_flashlightContainer {nullptr};
	bool m_flashlightOn {false};
};

} // namespace System::Apps::Tools
