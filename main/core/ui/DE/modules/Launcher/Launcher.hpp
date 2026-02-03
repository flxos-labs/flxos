#pragma once
#include "lvgl.h"
#include <string>
#include <vector>

namespace UI::Modules {

class Launcher {
public:

	Launcher(lv_obj_t* parent, lv_obj_t* dock, lv_event_cb_t appClickCb, void* userData);
	~Launcher() = default;

	lv_obj_t* getObj() const { return m_panel; }

private:

	void create();

	lv_obj_t* m_parent;
	lv_obj_t* m_dock;
	lv_event_cb_t m_appClickCb;
	void* m_userData;
	lv_obj_t* m_panel = nullptr;
	lv_obj_t* m_list = nullptr;
};

} // namespace UI::Modules
