#pragma once

#include "lvgl.h"
#include <flx/core/Singleton.hpp>
#include <vector>

namespace System {

/**
 * @brief Centralized focus management for windows and panels.
 *
 * Manages Z-order, visual feedback, and exclusive visibility.
 */
class FocusManager : public flx::Singleton<FocusManager> {
	friend class flx::Singleton<FocusManager>;

public:

	/**
	 * @brief Initialize the focus manager.
	 * @param window_container The container for app windows.
	 * @param screen The root screen object.
	 * @param status_bar Pointer to the status bar.
	 * @param dock Pointer to the dock.
	 */
	void init(lv_obj_t* window_container, lv_obj_t* screen, lv_obj_t* status_bar, lv_obj_t* dock);

	/**
	 * @brief Register a panel to be managed.
	 * @param panel The panel object (launcher, quick access, notification, etc.)
	 */
	void registerPanel(lv_obj_t* panel);

	/**
	 * @brief Register a window for event-based focus management.
	 * @param win The window object.
	 */
	void registerWindow(lv_obj_t* win);

	/**
	 * @brief Activates a window, bringing it to front and updating visual state.
	 * @param win The window to activate.
	 */
	void activateWindow(lv_obj_t* win);

	/**
	 * @brief Activates a panel, hiding any other open panels.
	 * @param panel The panel to activate.
	 */
	void activatePanel(lv_obj_t* panel);

	/**
	 * @brief Toggles a panel's visibility.
	 * @param panel The panel to toggle.
	 */
	void togglePanel(lv_obj_t* panel);

	/**
	 * @brief Dismisses all registered panels.
	 */
	void dismissAllPanels();

	/**
	 * @brief Set the notification panel for gesture access.
	 * @param panel The notification panel object.
	 */
	void setNotificationPanel(lv_obj_t* panel);

	/**
	 * @brief Returns the currently active window.
	 */
	lv_obj_t* getActiveWindow() const { return m_activeWindow; }

private:

	FocusManager();
	~FocusManager() = default;

	lv_obj_t* m_windowContainer = nullptr;
	lv_obj_t* m_screen = nullptr;
	lv_obj_t* m_statusBar = nullptr;
	lv_obj_t* m_dock = nullptr;
	lv_obj_t* m_activeWindow = nullptr;
	lv_obj_t* m_notificationPanel = nullptr;

	std::vector<lv_obj_t*> m_panels {};

	static void on_global_press(lv_event_t* e);
	static void on_global_release(lv_event_t* e);
	static void on_focus_event(lv_event_t* e);

	// Swipe gesture tracking
	int32_t m_swipeStartY = 0;
	bool m_swipeTracking = false;
};

} // namespace System
