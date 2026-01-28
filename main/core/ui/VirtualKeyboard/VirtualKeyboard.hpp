#pragma once

#include "lvgl.h"

class VirtualKeyboard {
public:

	static VirtualKeyboard& getInstance();

	void init();
	void register_input_area(lv_obj_t* obj);

private:

	VirtualKeyboard();
	~VirtualKeyboard() = default;

	lv_obj_t* m_keyboard;
	lv_obj_t* m_current_ta;

	static void on_ta_event(lv_event_t* e);
	static void on_kb_event(lv_event_t* e);
};
