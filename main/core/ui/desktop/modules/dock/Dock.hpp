#pragma once
#include "lvgl.h"
#include <functional>

namespace UI::Modules {

class Dock {
public:

	struct Callbacks {
		std::function<void()> onStartClick {};
		std::function<void()> onUpClick {};
	};

	Dock(lv_obj_t* parent, Callbacks callbacks);
	~Dock() = default;

	lv_obj_t* getObj() const { return m_dock; }
	lv_obj_t* getAppContainer() const { return m_appContainer; }

	static lv_obj_t* create_dock_btn(lv_obj_t* parent, const char* icon, int32_t w, int32_t h);

private:

	void create();

	lv_obj_t* m_parent;
	Callbacks m_callbacks;
	lv_obj_t* m_dock = nullptr;
	lv_obj_t* m_appContainer = nullptr;
};

} // namespace UI::Modules
