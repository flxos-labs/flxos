#pragma once
#include "lvgl.h"

namespace UI::Modules {

class StatusBar {
public:

	explicit StatusBar(lv_obj_t* parent);
	~StatusBar();

	lv_obj_t* getObj() const { return m_statusBar; }

private:

	void create();

	lv_obj_t* m_parent;
	lv_obj_t* m_statusBar = nullptr;
	lv_obj_t* m_timeLabel = nullptr;
	lv_timer_t* m_timer = nullptr;
};

} // namespace UI::Modules
