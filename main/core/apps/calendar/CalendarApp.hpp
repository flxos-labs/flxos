#pragma once

#include "lvgl.h"
#include <flx/apps/AppManager.hpp>
#include <flx/apps/AppManifest.hpp>

// Use flx::apps namespace elements
using flx::apps::App;
using flx::apps::AppManifest;

namespace System::Apps {

class CalendarApp : public flx::apps::App {
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

	static const AppManifest manifest;

private:

	lv_obj_t* m_container {nullptr};
	lv_obj_t* m_calendar {nullptr};

	void updateCalendarToday();
};

} // namespace System::Apps
