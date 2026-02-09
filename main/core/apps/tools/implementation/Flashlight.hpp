#pragma once

#include "lvgl.h"
#include <functional>

namespace System::Apps::Tools {

class Flashlight {
public:

	Flashlight() = default;
	~Flashlight() = default;

	void createView(lv_obj_t* parent, std::function<void()> onBack);
	lv_obj_t* getView() const { return m_view; }
	void show();
	void hide();
	void destroy();

private:

	lv_obj_t* m_view {nullptr};
	lv_obj_t* m_flashlightContainer {nullptr};
	bool m_flashlightOn {false};
};

} // namespace System::Apps::Tools
