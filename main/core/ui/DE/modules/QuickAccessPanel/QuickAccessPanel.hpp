#pragma once
#include "lvgl.h"

namespace UI::Modules {

class QuickAccessPanel {
public:

	QuickAccessPanel(lv_obj_t* parent, lv_obj_t* dock);
	~QuickAccessPanel() = default;

	lv_obj_t* getObj() const { return m_panel; }

private:

	void create();

	lv_obj_t* m_parent;
	lv_obj_t* m_dock;
	lv_obj_t* m_panel = nullptr;
	lv_obj_t* m_themeLabel = nullptr;
};

} // namespace UI::Modules
