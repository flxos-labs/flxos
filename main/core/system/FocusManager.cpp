#include "FocusManager.hpp"
#include "core/common/Logger.hpp"

namespace System {

FocusManager& FocusManager::getInstance() {
	static FocusManager instance;
	return instance;
}

FocusManager::FocusManager() {}

void FocusManager::init(lv_obj_t* window_container, lv_obj_t* screen, lv_obj_t* status_bar, lv_obj_t* dock) {
	m_windowContainer = window_container;
	m_screen = screen;
	m_statusBar = status_bar;
	m_dock = dock;

	// Register global event listener on all input devices
	lv_indev_t* indev = lv_indev_get_next(nullptr);
	while (indev) {
		lv_indev_add_event_cb(indev, on_global_press, LV_EVENT_PRESSED, this);
		lv_indev_add_event_cb(indev, on_global_release, LV_EVENT_RELEASED, this);
		indev = lv_indev_get_next(indev);
	}
}

void FocusManager::registerPanel(lv_obj_t* panel) {
	if (panel) {
		m_panels.push_back(panel);
		// Add event callbacks for defocus events (like dropdown does)
		lv_obj_add_event_cb(panel, on_focus_event, LV_EVENT_DEFOCUSED, this);
		lv_obj_add_event_cb(panel, on_focus_event, LV_EVENT_LEAVE, this);
	}
}

void FocusManager::registerWindow(lv_obj_t* win) {
	if (win) {
		// Make window focusable and add focus event handler
		lv_obj_add_flag(win, LV_OBJ_FLAG_CLICK_FOCUSABLE);
		lv_obj_add_event_cb(win, on_focus_event, LV_EVENT_FOCUSED, this);
	}
}

void FocusManager::activateWindow(lv_obj_t* win) {
	if (!win || !lv_obj_is_valid(win)) {
		return;
	}

	m_activeWindow = win;
	Log::info("FocusManager", "Activating window...");

	// Bring target to foreground
	lv_obj_move_to_index(win, -1);

	// Update states for all windows in the container
	uint32_t cnt = lv_obj_get_child_count(m_windowContainer);
	for (uint32_t i = 0; i < cnt; i++) {
		lv_obj_t* child = lv_obj_get_child(m_windowContainer, i);
		if (!lv_obj_is_valid(child) || lv_obj_get_user_data(child) == nullptr)
			continue;

		bool is_active = (child == win);
		lv_obj_t* dock_btn = (lv_obj_t*)lv_obj_get_user_data(child);

		if (is_active) {
			lv_obj_add_state(child, LV_STATE_FOCUSED);
			lv_obj_set_style_border_width(child, lv_dpx(2), 0);
			lv_obj_set_style_border_opa(child, LV_OPA_COVER, 0);
			if (dock_btn && lv_obj_is_valid(dock_btn))
				lv_obj_add_state(dock_btn, LV_STATE_CHECKED);
		} else {
			lv_obj_remove_state(child, LV_STATE_FOCUSED);
			lv_obj_set_style_border_width(child, lv_dpx(1), 0);
			lv_obj_set_style_border_opa(child, LV_OPA_40, 0);
			if (dock_btn && lv_obj_is_valid(dock_btn))
				lv_obj_remove_state(dock_btn, LV_STATE_CHECKED);
		}
		lv_obj_invalidate(child);
	}
}

void FocusManager::activatePanel(lv_obj_t* panel) {
	if (!panel) return;

	Log::debug("FocusManager", "Activating panel: %p", panel);

	// Hide other panels
	for (auto* p: m_panels) {
		if (p != panel && lv_obj_is_valid(p)) {
			lv_obj_add_flag(p, LV_OBJ_FLAG_HIDDEN);
			// Remove focus from hidden panels
			lv_obj_remove_flag(p, LV_OBJ_FLAG_CLICK_FOCUSABLE);
		}
	}

	// Show and focus the active panel
	lv_obj_clear_flag(panel, LV_OBJ_FLAG_HIDDEN);
	// Make panel focusable and focus it (like dropdown does when opening)
	lv_obj_add_flag(panel, LV_OBJ_FLAG_CLICK_FOCUSABLE);
	lv_group_t* g = lv_group_get_default();
	if (g) {
		lv_group_focus_obj(panel);
	}
}

void FocusManager::togglePanel(lv_obj_t* panel) {
	if (!panel) return;

	if (lv_obj_has_flag(panel, LV_OBJ_FLAG_HIDDEN)) {
		Log::debug("FocusManager", "Toggling panel ON: %p", panel);
		activatePanel(panel);
	} else {
		Log::debug("FocusManager", "Toggling panel OFF: %p", panel);
		lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);
		lv_obj_remove_flag(panel, LV_OBJ_FLAG_CLICK_FOCUSABLE);
	}
}

void FocusManager::dismissAllPanels() {
	Log::debug("FocusManager", "Dismissing all panels");
	for (auto* p: m_panels) {
		if (lv_obj_is_valid(p)) {
			lv_obj_add_flag(p, LV_OBJ_FLAG_HIDDEN);
			lv_obj_remove_flag(p, LV_OBJ_FLAG_CLICK_FOCUSABLE);
		}
	}
}

void FocusManager::setNotificationPanel(lv_obj_t* panel) {
	m_notificationPanel = panel;
}

void FocusManager::on_global_press(lv_event_t* e) {
	FocusManager* fm = (FocusManager*)lv_event_get_user_data(e);
	if (!fm) return;

	// Get touch point for swipe tracking
	lv_indev_t* indev = lv_event_get_indev(e);
	if (indev) {
		lv_point_t point;
		lv_indev_get_point(indev, &point);
		lv_coord_t screen_height = lv_display_get_vertical_resolution(lv_display_get_default());
		// Start tracking if touch started in top 15% of screen
		if (point.y < screen_height / 7) {
			fm->m_swipeStartY = point.y;
			fm->m_swipeTracking = true;
		} else {
			fm->m_swipeTracking = false;
		}
	}

	lv_obj_t* target = lv_indev_get_active_obj();
	if (!target) return;

	// 1. Check if we clicked inside a panel
	lv_obj_t* obj = target;
	while (obj) {
		for (auto* p: fm->m_panels) {
			if (p == obj) return; // Inside panel -> do nothing
		}
		obj = lv_obj_get_parent(obj);
	}

	// 2. If we are here, we are NOT in a panel. Dismiss them.
	fm->dismissAllPanels();

	// 3. Check if we clicked inside a window (to activate it)
	obj = target;
	while (obj) {
		lv_obj_t* parent = lv_obj_get_parent(obj);
		if (parent == fm->m_windowContainer || parent == fm->m_screen) {
			if (obj != fm->m_statusBar && obj != fm->m_dock && obj != fm->m_windowContainer) {
				// Only activate if it has user_data (real windows have dock buttons stored)
				if (lv_obj_get_user_data(obj) != nullptr) {
					fm->activateWindow(obj);
					return;
				}
			}
		}
		obj = parent;
	}
}

void FocusManager::on_focus_event(lv_event_t* e) {
	FocusManager* fm = (FocusManager*)lv_event_get_user_data(e);
	lv_obj_t* obj = (lv_obj_t*)lv_event_get_current_target(e);
	lv_event_code_t code = lv_event_get_code(e);

	if (!fm || !obj) return;

	// Handle window focus events
	if (code == LV_EVENT_FOCUSED) {
		// Check if this is a panel (panels shouldn't trigger window activation)
		bool is_panel = false;
		for (auto* p: fm->m_panels) {
			if (p == obj) {
				is_panel = true;
				break;
			}
		}
		if (!is_panel) {
			fm->activateWindow(obj);
		}
	}
	// Handle panel defocus events (like dropdown)
	else if (code == LV_EVENT_DEFOCUSED || code == LV_EVENT_LEAVE) {
		lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
		lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE);
	}
}

void FocusManager::on_global_release(lv_event_t* e) {
	FocusManager* fm = (FocusManager*)lv_event_get_user_data(e);
	if (!fm || !fm->m_swipeTracking || !fm->m_notificationPanel) return;

	lv_indev_t* indev = lv_event_get_indev(e);
	if (indev) {
		lv_point_t point;
		lv_indev_get_point(indev, &point);
		lv_coord_t delta = point.y - fm->m_swipeStartY;

		// Swipe down detected if moved at least 30 pixels down
		if (delta > 30) {
			fm->activatePanel(fm->m_notificationPanel);
		}
	}
	fm->m_swipeTracking = false;
}

} // namespace System
