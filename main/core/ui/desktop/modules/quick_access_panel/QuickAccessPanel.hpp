#pragma once

#include "core/lv_obj.h"

namespace UI::Modules {

class QuickAccessPanel {
public:

	QuickAccessPanel(lv_obj_t* parent, lv_obj_t* dock);
	~QuickAccessPanel();

	void create();

	// Renamed from getPanel() to getObj() to match Desktop.cpp
	lv_obj_t* getObj() const { return m_panel; }

private:

	bool setupPanel();
	void buildHeader();
	void buildToggles();
	void buildBrightnessSlider();

	lv_obj_t* m_parent;
	lv_obj_t* m_dock;
	lv_obj_t* m_panel {nullptr};
	lv_obj_t* m_themeLabel {nullptr};
};

} // namespace UI::Modules