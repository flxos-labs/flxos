#include "WM.hpp"

#include "../DE.hpp"
#include "core/apps/AppManager.hpp"
#include "core/tasks/gui/GuiTask.hpp"

WM& WM::getInstance() {
	static WM instance;
	return instance;
}

WM::WM()
	: m_windowContainer(nullptr), m_appContainer(nullptr), m_screen(nullptr),
	  m_statusBar(nullptr), m_dock(nullptr), m_fullScreenWindow(nullptr) {
	m_tiledWindows.clear();
}

WM::~WM() {}

void WM::init(lv_obj_t* window_container, lv_obj_t* app_container, lv_obj_t* screen, lv_obj_t* status_bar, lv_obj_t* dock) {
	m_windowContainer = window_container;
	m_appContainer = app_container;
	m_screen = screen;
	m_statusBar = status_bar;
	m_dock = dock;

	lv_obj_set_style_pad_all(m_windowContainer, lv_dpx(4), 0);
}

void WM::activate_window(lv_obj_t* target_win) {
	if (!target_win)
		return;
	// No internal locking to prevent deadlocks when called from locked
	// contexts Bring target to foreground within container
	lv_obj_move_to_index(target_win, -1);

	// Update states for all windows in the container
	uint32_t cnt = lv_obj_get_child_count(getInstance().m_windowContainer);
	for (uint32_t i = 0; i < cnt; i++) {
		lv_obj_t* win = lv_obj_get_child(getInstance().m_windowContainer, i);
		if (!lv_obj_is_valid(win) || lv_obj_get_user_data(win) == nullptr)
			continue; // Not a window

		bool is_active = (win == target_win);
		lv_obj_t* dock_btn = (lv_obj_t*)lv_obj_get_user_data(win);

		if (is_active) {
			lv_obj_add_state(win, LV_STATE_FOCUSED);
			lv_obj_set_style_border_width(win, lv_dpx(2), 0);
			lv_obj_set_style_border_opa(win, LV_OPA_COVER, 0);
			if (dock_btn && lv_obj_is_valid(dock_btn))
				lv_obj_add_state(dock_btn, LV_STATE_CHECKED);
		} else {
			lv_obj_remove_state(win, LV_STATE_FOCUSED);
			lv_obj_set_style_border_width(win, lv_dpx(1), 0);
			lv_obj_set_style_border_opa(win, LV_OPA_40, 0);
			if (dock_btn && lv_obj_is_valid(dock_btn))
				lv_obj_remove_state(dock_btn, LV_STATE_CHECKED);
		}
		lv_obj_invalidate(win);
	}
}

void WM::openApp(const std::string& packageName) {
	GuiTask::lock();

	// Check if app is already open
	for (const auto& pair: m_windowAppMap) {
		if (pair.second == packageName) {
			lv_obj_t* win = pair.first;
			lv_obj_remove_flag(win, LV_OBJ_FLAG_HIDDEN);
			activate_window(win);
			System::Apps::AppManager::getInstance().startApp(packageName);
			GuiTask::unlock();
			return;
		}
	}

	auto app =
		System::Apps::AppManager::getInstance().getAppByPackageName(packageName);
	if (!app) {
		GuiTask::unlock();
		return;
	}

	// Create new window
	lv_obj_t* win = lv_win_create(m_windowContainer);
	// Initial size will be handled by updateLayout, but set defaults just
	// in case
	lv_obj_set_style_radius(win, lv_dpx(8), 0);
	lv_obj_set_style_border_post(win, true, 0);

	m_windowAppMap[win] = packageName;
	m_tiledWindows.push_back(win);

	const char* iconSymbol = (const char*)app->getIcon();
	lv_obj_t* dock_btn =
		DE::create_dock_btn(m_appContainer, iconSymbol, lv_pct(15), lv_pct(85));
	lv_obj_set_style_bg_opa(dock_btn, LV_OPA_TRANSP, 0);
	lv_obj_set_style_bg_opa(dock_btn, LV_OPA_80, LV_STATE_CHECKED);
	lv_obj_add_event_cb(dock_btn, on_win_minimize, LV_EVENT_CLICKED, this);

	lv_obj_set_user_data(win, dock_btn);
	lv_obj_set_user_data(dock_btn, win);

	lv_obj_t* header = lv_win_get_header(win);
	lv_obj_set_height(header, lv_pct(10));
	lv_obj_set_style_pad_all(header, 0, 0);
	lv_obj_add_flag(header, LV_OBJ_FLAG_EVENT_BUBBLE);
	lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);
	lv_win_add_title(win, app->getAppName().c_str());

	lv_obj_t* min_btn = lv_win_add_button(win, LV_SYMBOL_DOWN, lv_pct(10));
	lv_obj_add_event_cb(min_btn, on_header_minimize, LV_EVENT_CLICKED, this);

	lv_obj_t* max_btn = lv_win_add_button(win, LV_SYMBOL_PLUS, lv_pct(10));
	if (lv_obj_get_child_count(max_btn) > 0) {
		m_windowMaxBtnLabelMap[win] = lv_obj_get_child(max_btn, 0);
	}
	lv_obj_add_event_cb(max_btn, on_win_maximize, LV_EVENT_CLICKED, this);

	lv_obj_t* close_btn = lv_win_add_button(win, LV_SYMBOL_CLOSE, lv_pct(10));
	lv_obj_add_event_cb(close_btn, on_win_close, LV_EVENT_CLICKED, this);

	lv_obj_t* content = lv_win_get_content(win);
	lv_obj_set_style_pad_all(content, 0, 0);
	lv_obj_add_flag(content, LV_OBJ_FLAG_EVENT_BUBBLE);

	app->createUI(content);

	lv_obj_add_event_cb(win, on_win_focus, LV_EVENT_PRESSED, this);
	activate_window(win);

	System::Apps::AppManager::getInstance().startApp(app);
	updateLayout(); // Layout updates after window creation
	GuiTask::unlock();
}

void WM::closeApp(const std::string& packageName) {
	GuiTask::lock();
	lv_obj_t* winToClose = nullptr;
	for (const auto& pair: m_windowAppMap) {
		if (pair.second == packageName) {
			winToClose = pair.first;
			break;
		}
	}
	if (winToClose)
		closeWindow_internal(winToClose);
	GuiTask::unlock();
}

void WM::on_win_focus(lv_event_t* e) {
	WM* wm = (WM*)lv_event_get_user_data(e);
	lv_obj_t* win = lv_event_get_current_target_obj(e);
	activate_window(win);

	if (wm && wm->m_windowAppMap.count(win)) {
		System::Apps::AppManager::getInstance().startApp(wm->m_windowAppMap[win]);
	}
}

void WM::on_win_minimize(lv_event_t* e) {
	WM* wm = (WM*)lv_event_get_user_data(e);
	lv_obj_t* db = lv_event_get_target_obj(e);
	if (!db || !wm)
		return;

	lv_obj_t* w = (lv_obj_t*)lv_obj_get_user_data(db);
	if (w && lv_obj_is_valid(w)) {
		if (w == wm->m_fullScreenWindow) {
			wm->toggleFullScreen(w);
		}
		bool is_hidden = lv_obj_has_flag(w, LV_OBJ_FLAG_HIDDEN);
		bool is_active =
			(lv_obj_get_style_border_width(w, LV_PART_MAIN) == lv_dpx(2));

		if (is_hidden) {
			lv_obj_remove_flag(w, LV_OBJ_FLAG_HIDDEN);
			activate_window(w);
		} else {
			if (!is_active) {
				activate_window(w);
			} else {
				lv_obj_add_flag(w, LV_OBJ_FLAG_HIDDEN);
				lv_obj_remove_state(db, LV_STATE_CHECKED);
			}
		}
		// Update layout after minimizing or restoring
		wm->updateLayout();
	}
}

void WM::on_header_minimize(lv_event_t* e) {
	WM* wm = (WM*)lv_event_get_user_data(e);
	lv_obj_t* btn = lv_event_get_target_obj(e);
	lv_obj_t* header = lv_obj_get_parent(btn);
	lv_obj_t* w = lv_obj_get_parent(header);

	if (w) {
		if (wm && w == wm->m_fullScreenWindow) {
			wm->toggleFullScreen(w);
		}
		lv_obj_t* dock_btn = (lv_obj_t*)lv_obj_get_user_data(w);
		lv_obj_add_flag(w, LV_OBJ_FLAG_HIDDEN);
		if (dock_btn)
			lv_obj_remove_state(dock_btn, LV_STATE_CHECKED);
		wm->updateLayout();
	}
}

void WM::closeWindow(lv_obj_t* w) {
	if (!w)
		return;
	GuiTask::lock();
	closeWindow_internal(w);
	GuiTask::unlock();
}

void WM::closeWindow_internal(lv_obj_t* w) {
	if (!w || !lv_obj_is_valid(w))
		return;
	// Assumes GuiTask lock is already held

	if (w == m_fullScreenWindow) {
		toggleFullScreen(w);
	}

	if (m_windowAppMap.count(w)) {
		std::string pkg = m_windowAppMap[w];
		m_windowAppMap.erase(w);
		System::Apps::AppManager::getInstance().stopApp(pkg);
	}

	if (m_windowMaxBtnLabelMap.count(w)) {
		m_windowMaxBtnLabelMap.erase(w);
	}

	lv_obj_t* db = (lv_obj_t*)lv_obj_get_user_data(w);
	if (db && lv_obj_is_valid(db))
		lv_obj_delete(db);

	// Remove from tiled list
	for (auto it = m_tiledWindows.begin(); it != m_tiledWindows.end(); ++it) {
		if (*it == w) {
			m_tiledWindows.erase(it);
			break;
		}
	}
	lv_obj_delete(w);
	updateLayout();
}

void WM::on_win_close(lv_event_t* e) {
	WM* wm = (WM*)lv_event_get_user_data(e);
	lv_obj_t* btn = lv_event_get_target_obj(e);
	lv_obj_t* header = lv_obj_get_parent(btn);
	lv_obj_t* w = lv_obj_get_parent(header);
	if (wm)
		// Runs in GUI thread implicitly, but to be sure we treat it as
		// internal logic However, if we call internal() we assume lock
		// is held. Callbacks are IN the lock.
		wm->closeWindow_internal(w);
}

void WM::on_win_maximize(lv_event_t* e) {
	WM* wm = (WM*)lv_event_get_user_data(e);
	lv_obj_t* btn = lv_event_get_target_obj(e);
	lv_obj_t* header = lv_obj_get_parent(btn);
	lv_obj_t* w = lv_obj_get_parent(header);

	if (wm)
		wm->toggleFullScreen(w);
}

void WM::toggleFullScreen(lv_obj_t* win) {
	// Internal helper, no lock
	lv_obj_t* max_btn_content = nullptr;
	if (m_windowMaxBtnLabelMap.count(win)) {
		max_btn_content = m_windowMaxBtnLabelMap[win];
	}

	if (m_fullScreenWindow == win) {
		lv_obj_set_parent(win, m_windowContainer);
		lv_obj_remove_flag(win, LV_OBJ_FLAG_HIDDEN);
		lv_obj_remove_flag(m_statusBar, LV_OBJ_FLAG_HIDDEN);
		lv_obj_remove_flag(m_dock, LV_OBJ_FLAG_HIDDEN);
		m_fullScreenWindow = nullptr;
		if (max_btn_content) {
			lv_image_set_src(max_btn_content, LV_SYMBOL_PLUS);
		}
	} else {
		if (m_fullScreenWindow) {
			// Recursively toggle off existing full screen
			toggleFullScreen(m_fullScreenWindow);
		}

		lv_obj_set_parent(win, m_screen);
		lv_obj_set_size(win, lv_pct(100), lv_pct(100));
		lv_obj_set_pos(win, 0, 0);
		lv_obj_add_flag(m_statusBar, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(m_dock, LV_OBJ_FLAG_HIDDEN);
		lv_obj_move_to_index(win, -1);
		m_fullScreenWindow = win;
		if (max_btn_content) {
			lv_image_set_src(max_btn_content, LV_SYMBOL_MINUS);
		}
	}
	activate_window(win);
	updateLayout();
}

void WM::updateLayout() {
	if (!m_windowContainer || !m_screen)
		return;

	// Force layout update on the parent (screen) to ensure
	// m_windowContainer has updated size after toggling status_bar or dock
	// visibility.
	lv_obj_update_layout(m_screen);

	std::vector<lv_obj_t*> visibleWins;
	for (auto* w: m_tiledWindows) {
		// Only layout windows that are in the container and not hidden
		if (w && lv_obj_is_valid(w) && !lv_obj_has_flag(w, LV_OBJ_FLAG_HIDDEN) &&
			lv_obj_get_parent(w) == m_windowContainer) {
			visibleWins.push_back(w);
		}
	}

	if (visibleWins.empty())
		return;

	lv_coord_t w_avail = lv_obj_get_content_width(m_windowContainer);
	lv_coord_t h_avail = lv_obj_get_content_height(m_windowContainer);

	lv_area_t rect;
	rect.x1 = 0;
	rect.y1 = 0;
	rect.x2 = w_avail;
	rect.y2 = h_avail;

	bool split_vertical =
		true; // Start with identifying Left vs Right split (vertical line)
	int gap = lv_dpx(4);

	for (size_t i = 0; i < visibleWins.size(); ++i) {
		lv_obj_t* win = visibleWins[i];

		// If it's the last window, give it the remaining space
		if (i == visibleWins.size() - 1) {
			lv_obj_set_pos(win, rect.x1, rect.y1);
			lv_obj_set_size(win, rect.x2 - rect.x1, rect.y2 - rect.y1);
		} else {
			lv_area_t first_half = rect;
			lv_area_t second_half = rect;

			if (split_vertical) {
				lv_coord_t w_half = (rect.x2 - rect.x1) / 2;
				first_half.x2 = rect.x1 + w_half - (gap / 2);
				second_half.x1 = rect.x1 + w_half + (gap / 2);
			} else {
				lv_coord_t h_half = (rect.y2 - rect.y1) / 2;
				first_half.y2 = rect.y1 + h_half - (gap / 2);
				second_half.y1 = rect.y1 + h_half + (gap / 2);
			}

			lv_obj_set_pos(win, first_half.x1, first_half.y1);
			lv_obj_set_size(win, first_half.x2 - first_half.x1, first_half.y2 - first_half.y1);

			rect = second_half;
			split_vertical = !split_vertical;
		}
		// Debug or refresh if needed
		lv_obj_invalidate(win);
	}
}