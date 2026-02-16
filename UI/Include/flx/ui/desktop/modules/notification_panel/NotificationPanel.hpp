#pragma once
#include "lvgl.h"
#include <memory>
#include <string>

namespace UI::Modules {

class NotificationPanel {
public:

	NotificationPanel(lv_obj_t* parent, lv_obj_t* statusBar);
	~NotificationPanel();

	lv_obj_t* getObj() const { return m_panel; }
	lv_obj_t* getList() const { return m_list; }

	void update_list();

private:

	void create();
	static void on_clear_click(lv_event_t* e);
	static void on_close_notif_click(lv_event_t* e);

	lv_obj_t* m_parent;
	lv_obj_t* m_statusBar;
	lv_obj_t* m_panel = nullptr;
	lv_obj_t* m_list = nullptr;
	lv_obj_t* m_clearAllBtn = nullptr;
	size_t m_observerId = 0;
	std::shared_ptr<bool> m_alive = std::make_shared<bool>(true);
};

} // namespace UI::Modules
