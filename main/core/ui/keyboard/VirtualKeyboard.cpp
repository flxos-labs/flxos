#include "VirtualKeyboard.hpp"
#include "../theming/layout_constants/LayoutConstants.hpp"
#include "core/lv_obj.h"
#include "core/lv_obj_event.h"
#include "core/lv_obj_pos.h"
#include "core/lv_obj_scroll.h"
#include "core/lv_obj_style.h"
#include "display/lv_display.h"
#include "lv_api_map_v8.h"
#include "misc/lv_anim.h"
#include "misc/lv_area.h"
#include "misc/lv_event.h"
#include "misc/lv_types.h"
#include "widgets/keyboard/lv_keyboard.h"
#include <cstddef>
#include <flx/core/Logger.hpp>
#include <string_view>

static constexpr std::string_view TAG = "VirtualKeyboard";

VirtualKeyboard& VirtualKeyboard::getInstance() {
	static VirtualKeyboard instance;
	return instance;
}

VirtualKeyboard::VirtualKeyboard() {}

void VirtualKeyboard::init() {
	if (m_keyboard) {
		return;
	}

	Log::info(TAG, "Initializing virtual keyboard...");

	// Create keyboard on top layer to ensure visibility
	m_keyboard = lv_keyboard_create(lv_layer_top());

	// Set size to half screen or appropriate size
	lv_obj_set_size(m_keyboard, lv_pct(100), lv_pct(LayoutConstants::KEYBOARD_HEIGHT_PCT));

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
	if (!m_keyboard) {
		init();
	}

	// Add event callback to show/hide keyboard on focus
	lv_obj_add_event_cb(obj, on_ta_event, LV_EVENT_ALL, this);
}

void VirtualKeyboard::on_ta_event(lv_event_t* e) {
	auto* vk = (VirtualKeyboard*)lv_event_get_user_data(e);
	lv_event_code_t const code = lv_event_get_code(e);
	auto* ta = (lv_obj_t*)lv_event_get_target(e);

	if (code == LV_EVENT_FOCUSED || code == LV_EVENT_CLICKED) {
		// Show keyboard
		if (vk->m_keyboard) {
			Log::debug(TAG, "Showing keyboard for text area");
			lv_keyboard_set_textarea(vk->m_keyboard, ta);
			lv_obj_remove_flag(vk->m_keyboard, LV_OBJ_FLAG_HIDDEN);
			lv_obj_move_foreground(vk->m_keyboard);
			vk->m_current_ta = ta;

			// Scroll to view
			lv_obj_scroll_to_view(ta, LV_ANIM_ON);
		}
	} else if (code == LV_EVENT_DEFOCUSED) {
		if (vk->m_keyboard) {
			Log::debug(TAG, "Hiding keyboard (defocused)");
			lv_keyboard_set_textarea(vk->m_keyboard, nullptr);
			lv_obj_add_flag(vk->m_keyboard, LV_OBJ_FLAG_HIDDEN);
			vk->m_current_ta = nullptr;
		}
	} else if (code == LV_EVENT_DELETE) {
		if (vk->m_current_ta == ta) {
			if (vk->m_keyboard) {
				lv_keyboard_set_textarea(vk->m_keyboard, nullptr);
				lv_obj_add_flag(vk->m_keyboard, LV_OBJ_FLAG_HIDDEN);
			}
			vk->m_current_ta = nullptr;
		}
	}
}

void VirtualKeyboard::on_kb_event(lv_event_t* e) {
	auto* vk = (VirtualKeyboard*)lv_event_get_user_data(e);
	lv_event_code_t const code = lv_event_get_code(e);

	if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
		Log::debug(TAG, "Hiding keyboard (ready/cancel)");
		// Hide keyboard
		lv_obj_add_flag(vk->m_keyboard, LV_OBJ_FLAG_HIDDEN);
		if (vk->m_current_ta) {
			lv_obj_remove_state(vk->m_current_ta, LV_STATE_FOCUSED); // Optional: remove focus
			lv_keyboard_set_textarea(vk->m_keyboard, nullptr);
			vk->m_current_ta = nullptr;
		}
	}
}
