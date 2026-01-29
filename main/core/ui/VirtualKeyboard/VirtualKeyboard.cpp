#include "VirtualKeyboard.hpp"
#include "esp_log.h"

static const char* TAG = "VirtualKeyboard";

VirtualKeyboard& VirtualKeyboard::getInstance() {
	static VirtualKeyboard instance;
	return instance;
}

VirtualKeyboard::VirtualKeyboard()
	: m_keyboard(nullptr), m_current_ta(nullptr) {}

void VirtualKeyboard::init() {
	if (m_keyboard)
		return;

	ESP_LOGI(TAG, "Initializing Virtual Keyboard");

	// Create keyboard on top layer to ensure visibility
	m_keyboard = lv_keyboard_create(lv_layer_top());

	// Set size to half screen or appropriate size
	lv_obj_set_size(m_keyboard, lv_pct(100), lv_pct(40));

	// Align to bottom
	lv_obj_align(m_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);

	// Initially hidden
	lv_obj_add_flag(m_keyboard, LV_OBJ_FLAG_HIDDEN);

	// Handle keyboard events (like check/close button)
	lv_obj_add_event_cb(m_keyboard, on_kb_event, LV_EVENT_VALUE_CHANGED, this);
	lv_obj_add_event_cb(m_keyboard, on_kb_event, LV_EVENT_READY, this);
	lv_obj_add_event_cb(m_keyboard, on_kb_event, LV_EVENT_CANCEL, this);
}

void VirtualKeyboard::register_input_area(lv_obj_t* obj) {
	if (!m_keyboard)
		init();

	ESP_LOGD(TAG, "Registering input area for virtual keyboard: %p", obj);
	// Add event callback to show/hide keyboard on focus
	lv_obj_add_event_cb(obj, on_ta_event, LV_EVENT_ALL, this);
}

void VirtualKeyboard::on_ta_event(lv_event_t* e) {
	VirtualKeyboard* vk = (VirtualKeyboard*)lv_event_get_user_data(e);
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t* ta = (lv_obj_t*)lv_event_get_target(e);

	if (code == LV_EVENT_FOCUSED || code == LV_EVENT_CLICKED) {
		// Show keyboard
		if (vk->m_keyboard) {
			ESP_LOGD(TAG, "Textarea focused, showing keyboard");
			lv_keyboard_set_textarea(vk->m_keyboard, ta);
			lv_obj_remove_flag(vk->m_keyboard, LV_OBJ_FLAG_HIDDEN);
			lv_obj_move_foreground(vk->m_keyboard);
			vk->m_current_ta = ta;

			// Scroll to view
			lv_obj_scroll_to_view(ta, LV_ANIM_ON);
		}
	} else if (code == LV_EVENT_DEFOCUSED) {
		// Determine if we should hide.
		// If the focus went to the keyboard itself, we might want to keep it?
		// But usually LVGL keyboard doesn't take focus in a way that unfocuses TA
		// completely unless clicked outside. Actually, pressing "OK" on keyboard
		// sends CANCEL/READY.

		// We often hide on defocus, but let's check input device type or rely on KB
		// events
		lv_indev_t* indev = lv_indev_active();
		if (indev && lv_indev_get_type(indev) != LV_INDEV_TYPE_KEYPAD) {
			// For touch, if we clicked somewhere else, hide.
			// But if we clicked the keyboard buttons, the TA might lose focus?
			// Usually TA keeps focus when typing on virtual keyboard.
		}

		// Ideally, we hide when the user explicitly closes it or focuses something
		// else that is NOT the keyboard. Simple behavior: Hide when defocused.
		if (vk->m_keyboard) {
			ESP_LOGD(TAG, "Textarea defocused, hiding keyboard");
			lv_keyboard_set_textarea(vk->m_keyboard, NULL);
			lv_obj_add_flag(vk->m_keyboard, LV_OBJ_FLAG_HIDDEN);
			vk->m_current_ta = nullptr;
		}
	} else if (code == LV_EVENT_DELETE) {
		if (vk->m_current_ta == ta) {
			if (vk->m_keyboard) {
				lv_keyboard_set_textarea(vk->m_keyboard, NULL);
				lv_obj_add_flag(vk->m_keyboard, LV_OBJ_FLAG_HIDDEN);
			}
			vk->m_current_ta = nullptr;
		}
	}
}

void VirtualKeyboard::on_kb_event(lv_event_t* e) {
	VirtualKeyboard* vk = (VirtualKeyboard*)lv_event_get_user_data(e);
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
		// Hide keyboard
		lv_obj_add_flag(vk->m_keyboard, LV_OBJ_FLAG_HIDDEN);
		if (vk->m_current_ta) {
			lv_obj_remove_state(vk->m_current_ta,
								LV_STATE_FOCUSED); // Optional: remove focus
			lv_keyboard_set_textarea(vk->m_keyboard, NULL);
			vk->m_current_ta = nullptr;
		}
	}
}
