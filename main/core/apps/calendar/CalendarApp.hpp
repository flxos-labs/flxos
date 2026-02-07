#pragma once

#include "core/apps/AppManager.hpp"
#include "lvgl.h"

namespace System::Apps {

class CalendarApp : public App {
public:

	CalendarApp() = default;
	~CalendarApp() override = default;

	bool onStart() override;
	bool onResume() override;
	void onPause() override;
	void createUI(void* parent) override;
	void onStop() override;
	void update() override;

	std::string getPackageName() const override { return "com.flxos.calendar"; }
	std::string getAppName() const override { return "Calendar"; }
	const void* getIcon() const override { return LV_SYMBOL_LEFT; }

private:

	lv_obj_t* m_container {nullptr};
	lv_obj_t* m_calendar {nullptr};

	void updateCalendarToday();
};

} // namespace System::Apps
