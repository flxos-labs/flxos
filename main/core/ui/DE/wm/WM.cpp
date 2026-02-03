#include "WM.hpp"
#include "../../theming/LayoutConstants/LayoutConstants.hpp"
#include "../../theming/UiConstants/UiConstants.hpp"
#include "core/common/Logger.hpp"

#include "../DE.hpp"
#include "../modules/Dock/Dock.hpp"
#include "core/apps/AppManager.hpp"
#include "core/system/Display/DisplayManager.hpp"
#include "core/system/Focus/FocusManager.hpp"
#include "core/system/System/SystemManager.hpp"
#include "core/system/Theme/ThemeManager.hpp"
#include "core/tasks/gui/GuiTask.hpp"
#include <algorithm>
#include <string_view>

static constexpr std::string_view TAG = "WM";

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
	Log::info(TAG, "Initializing Window Manager...");
	m_windowContainer = window_container;
	m_appContainer = app_container;
	m_screen = screen;
	m_statusBar = status_bar;
	m_dock = dock;

	lv_obj_set_style_pad_all(m_windowContainer, lv_dpx(UiConstants::PAD_SMALL), 0);

	// Register as observer of AppManager for state synchronization
	System::Apps::AppManager::getInstance().addObserver(this);

	// Register rotation observer to update layout on display orientation changes
	lv_subject_add_observer(
		&System::DisplayManager::getInstance().getRotationSubject(),
		on_rotation_change,
		nullptr
	);

	// Initialize FocusManager with our containers
	System::FocusManager::getInstance().init(m_windowContainer, m_screen, m_statusBar, m_dock);
}

void WM::openApp(const std::string& packageName) {
	GuiTask::lock();

	// Check if app is already open
	if (lv_obj_t* win = findWindowByPackage(packageName)) {
		Log::info(TAG, "App %s already open, activating window", packageName.c_str());
		lv_obj_remove_flag(win, LV_OBJ_FLAG_HIDDEN);

		lv_obj_t* dp = (lv_obj_t*)lv_obj_get_user_data(win);
		if (dp && lv_obj_is_valid(dp)) lv_obj_add_state(dp, LV_STATE_USER_1);

		System::FocusManager::getInstance().activateWindow(win);
		System::Apps::AppManager::getInstance().startApp(packageName);
		GuiTask::unlock();
		return;
	}

	Log::info(TAG, "Opening app: %s", packageName.c_str());
	auto app = System::Apps::AppManager::getInstance().getAppByPackageName(packageName);
	if (!app) {
		Log::error(TAG, "Failed to open app: %s (not found)", packageName.c_str());
		GuiTask::unlock();
		return;
	}

	// Create new window
	lv_obj_t* win = lv_win_create(m_windowContainer);
	// Initial size will be handled by updateLayout, but set defaults just
	// in case
	lv_obj_set_style_radius(win, lv_dpx(UiConstants::RADIUS_DEFAULT), 0);
	lv_obj_set_style_border_post(win, true, 0);

	m_windowAppMap[win] = packageName;
	m_tiledWindows.push_back(win);

	const char* iconSymbol = (const char*)app->getIcon();
	lv_obj_t* dock_btn =
		UI::Modules::Dock::create_dock_btn(m_appContainer, iconSymbol, lv_pct(UiConstants::SIZE_DOCK_ICON_PCT), lv_pct(LayoutConstants::LIST_HEIGHT_PCT));

	// Dock button state styles:
	// Default (Minimized): Transparent
	// USER_1 (Open/Visible): Glass Opacity
	// CHECKED (Active): High Opacity (Active color handled by Theme)
	lv_obj_set_style_bg_opa(dock_btn, UiConstants::OPA_TRANSP, 0);
	lv_obj_set_style_bg_opa(dock_btn, UiConstants::OPA_GLASS_BG, LV_STATE_USER_1);
	lv_obj_set_style_bg_opa(dock_btn, UiConstants::OPA_HIGH, LV_STATE_CHECKED);

	// Add observer for transparency
	lv_subject_add_observer_obj(
		&System::ThemeManager::getInstance().getTransparencyEnabledSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			lv_obj_t* btn = lv_observer_get_target_obj(observer);
			bool enabled = lv_subject_get_int(subject);
			if (enabled) {
				lv_obj_set_style_bg_opa(btn, UiConstants::OPA_GLASS_BG, LV_STATE_USER_1);
			} else {
				lv_obj_set_style_bg_opa(btn, UiConstants::OPA_COVER, LV_STATE_USER_1);
			}
		},
		dock_btn, nullptr
	);

	lv_obj_add_state(dock_btn, LV_STATE_USER_1); // Window starts visible

	lv_obj_add_event_cb(dock_btn, on_win_minimize, LV_EVENT_CLICKED, this);

	lv_obj_set_user_data(win, dock_btn);
	lv_obj_set_user_data(dock_btn, win);

	lv_win_add_title(win, app->getAppName().c_str());
	lv_obj_t* header = lv_win_get_header(win);
	lv_obj_set_height(header, lv_pct(UiConstants::SIZE_WIN_HEADER_PCT));
	lv_obj_set_style_min_height(header, lv_dpx(UiConstants::SIZE_HEADER), 0);
	lv_obj_set_style_pad_all(header, 0, 0);
	lv_obj_add_flag(header, LV_OBJ_FLAG_EVENT_BUBBLE);
	lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t* min_btn = lv_win_add_button(win, LV_SYMBOL_DOWN, lv_pct(UiConstants::SIZE_WIN_HEADER_PCT));
	lv_obj_set_style_min_width(min_btn, lv_dpx(UiConstants::SIZE_HEADER), 0);
	lv_obj_add_event_cb(min_btn, on_header_minimize, LV_EVENT_CLICKED, this);

	lv_obj_t* max_btn = lv_win_add_button(win, LV_SYMBOL_PLUS, lv_pct(UiConstants::SIZE_WIN_HEADER_PCT));
	lv_obj_set_style_min_width(max_btn, lv_dpx(UiConstants::SIZE_HEADER), 0);
	if (lv_obj_get_child_count(max_btn) > 0) {
		m_windowMaxBtnLabelMap[win] = lv_obj_get_child(max_btn, 0);
	}
	lv_obj_add_event_cb(max_btn, on_win_maximize, LV_EVENT_CLICKED, this);

	lv_obj_t* close_btn = lv_win_add_button(win, LV_SYMBOL_CLOSE, lv_pct(UiConstants::SIZE_WIN_HEADER_PCT));
	lv_obj_set_style_min_width(close_btn, lv_dpx(UiConstants::SIZE_HEADER), 0);
	lv_obj_add_event_cb(close_btn, on_win_close, LV_EVENT_CLICKED, this);

	lv_obj_t* content = lv_win_get_content(win);
	lv_obj_set_style_pad_all(content, 0, 0);
	lv_obj_add_flag(content, LV_OBJ_FLAG_EVENT_BUBBLE);

	app->createUI(content);

	// Register window for event-based focus management
	System::FocusManager::getInstance().registerWindow(win);
	System::FocusManager::getInstance().activateWindow(win);

	System::Apps::AppManager::getInstance().startApp(app);
	updateLayout(); // Layout updates after window creation
	GuiTask::unlock();
}

void WM::closeApp(const std::string& packageName) {
	GuiTask::lock();
	if (lv_obj_t* win = findWindowByPackage(packageName)) {
		closeWindow_internal(win);
	} else {
	}
	GuiTask::unlock();
}

void WM::on_win_minimize(lv_event_t* e) {
	WM* wm = (WM*)lv_event_get_user_data(e);
	lv_obj_t* db = lv_event_get_target_obj(e);
	if (!db || !wm) return;

	lv_obj_t* w = (lv_obj_t*)lv_obj_get_user_data(db);
	if (!w || !lv_obj_is_valid(w)) return;

	if (w == wm->m_fullScreenWindow) wm->toggleFullScreen(w);

	bool is_hidden = lv_obj_has_flag(w, LV_OBJ_FLAG_HIDDEN);
	bool is_active = (lv_obj_get_style_border_width(w, LV_PART_MAIN) == lv_dpx(UiConstants::BORDER_FOCUS));

	if (is_hidden || !is_active) {
		lv_obj_remove_flag(w, LV_OBJ_FLAG_HIDDEN);
		// Restore visible state on dock btn
		if (db && lv_obj_is_valid(db)) lv_obj_add_state(db, LV_STATE_USER_1);

		System::FocusManager::getInstance().activateWindow(w);
		if (wm->m_windowAppMap.count(w))
			System::Apps::AppManager::getInstance().startApp(wm->m_windowAppMap[w]);
	} else {
		lv_obj_add_flag(w, LV_OBJ_FLAG_HIDDEN);
		lv_obj_remove_state(db, LV_STATE_CHECKED);
		lv_obj_remove_state(db, LV_STATE_USER_1); // Minimized
	}
	wm->updateLayout();
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
		if (dock_btn) {
			lv_obj_remove_state(dock_btn, LV_STATE_CHECKED);
			lv_obj_remove_state(dock_btn, LV_STATE_USER_1); // Minimized
		}
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

void WM::closeWindow_internal(lv_obj_t* win) {
	if (!win || !lv_obj_is_valid(win))
		return;

	Log::info(TAG, "Closing window: %p", win);
	// Assumes GuiTask lock is already held

	if (win == m_fullScreenWindow) {
		toggleFullScreen(win);
	}

	if (m_windowAppMap.count(win)) {
		std::string pkg = m_windowAppMap[win];
		m_windowAppMap.erase(win);
		System::Apps::AppManager::getInstance().stopApp(pkg, false);
	}

	m_windowMaxBtnLabelMap.erase(win);

	lv_obj_t* db = (lv_obj_t*)lv_obj_get_user_data(win);
	if (db && lv_obj_is_valid(db)) lv_obj_delete(db);

	m_tiledWindows.erase(std::remove(m_tiledWindows.begin(), m_tiledWindows.end(), win), m_tiledWindows.end());
	lv_obj_delete(win);
	updateLayout();
}

void WM::on_win_close(lv_event_t* e) {
	WM* wm = (WM*)lv_event_get_user_data(e);
	lv_obj_t* w = getWindowFromHeaderBtn(e);
	if (wm) wm->closeWindow_internal(w);
}

void WM::on_win_maximize(lv_event_t* e) {
	WM* wm = (WM*)lv_event_get_user_data(e);
	lv_obj_t* w = getWindowFromHeaderBtn(e);
	if (wm) wm->toggleFullScreen(w);
}

void WM::toggleFullScreen(lv_obj_t* win) {
	auto it = m_windowMaxBtnLabelMap.find(win);
	lv_obj_t* max_btn_content = (it != m_windowMaxBtnLabelMap.end()) ? it->second : nullptr;

	if (m_fullScreenWindow == win) {
		lv_obj_set_parent(win, m_windowContainer);
		lv_obj_remove_flag(win, LV_OBJ_FLAG_HIDDEN);
		lv_obj_remove_flag(m_statusBar, LV_OBJ_FLAG_HIDDEN);
		lv_obj_remove_flag(m_dock, LV_OBJ_FLAG_HIDDEN);
		m_fullScreenWindow = nullptr;
		if (max_btn_content) lv_image_set_src(max_btn_content, LV_SYMBOL_PLUS);
	} else {
		if (m_fullScreenWindow) toggleFullScreen(m_fullScreenWindow);
		lv_obj_set_parent(win, m_screen);
		lv_obj_set_size(win, lv_pct(100), lv_pct(100));
		lv_obj_set_pos(win, 0, 0);
		lv_obj_add_flag(m_statusBar, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(m_dock, LV_OBJ_FLAG_HIDDEN);
		lv_obj_move_to_index(win, -1);
		m_fullScreenWindow = win;
		if (max_btn_content) lv_image_set_src(max_btn_content, LV_SYMBOL_MINUS);
	}
	System::FocusManager::getInstance().activateWindow(win);
	Log::info(TAG, "Toggled FullScreen for win: %p (active: %s)", win, m_fullScreenWindow == win ? "YES" : "NO");
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
	std::copy_if(m_tiledWindows.begin(), m_tiledWindows.end(), std::back_inserter(visibleWins), [this](lv_obj_t* w) {
		return w && lv_obj_is_valid(w) && !lv_obj_has_flag(w, LV_OBJ_FLAG_HIDDEN) &&
			lv_obj_get_parent(w) == m_windowContainer;
	});
	Log::debug(TAG, "Updating layout for %zu visible windows", visibleWins.size());
	if (visibleWins.empty()) return;

	int32_t w_avail = lv_obj_get_content_width(m_windowContainer);
	int32_t h_avail = lv_obj_get_content_height(m_windowContainer);

	lv_area_t rect;
	rect.x1 = 0;
	rect.y1 = 0;
	rect.x2 = w_avail;
	rect.y2 = h_avail;

	bool split_vertical =
		true; // Start with identifying Left vs Right split (vertical line)
	int gap = lv_dpx(UiConstants::PAD_SMALL);

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
				int32_t w_half = (rect.x2 - rect.x1) / 2;
				first_half.x2 = rect.x1 + w_half - (gap / 2);
				second_half.x1 = rect.x1 + w_half + (gap / 2);
			} else {
				int32_t h_half = (rect.y2 - rect.y1) / 2;
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
void WM::on_rotation_change(lv_observer_t* observer, lv_subject_t* subject) {
	// Force a layout update for all tiled windows
	WM::getInstance().updateLayout();
}

// AppStateObserver implementation
void WM::onAppStarted(const std::string& packageName) {
	// Currently no action needed when app starts
	// Window is already created via openApp() before app starts
}

void WM::onAppStopped(const std::string& packageName) {
	// App was stopped, ensure window is closed if it exists
	// This handles the case where app is stopped programmatically
	if (hasWindowForApp(packageName)) {
		closeApp(packageName);
	}
}

bool WM::hasWindowForApp(const std::string& packageName) const {
	return findWindowByPackage(packageName) != nullptr;
}

lv_obj_t* WM::findWindowByPackage(const std::string& packageName) const {
	for (const auto& [win, pkg]: m_windowAppMap)
		if (pkg == packageName) return win;
	return nullptr;
}

lv_obj_t* WM::getWindowFromHeaderBtn(lv_event_t* e) {
	lv_obj_t* btn = lv_event_get_target_obj(e);
	return lv_obj_get_parent(lv_obj_get_parent(btn));
}
