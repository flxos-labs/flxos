#include "CalendarApp.hpp"
#include <ctime>
#include <flx/core/Logger.hpp>

namespace System::Apps {

static constexpr const char* TAG = "CalendarApp";

bool CalendarApp::onStart() {
	Log::info(TAG, "Calendar app started");
	return true;
}

bool CalendarApp::onResume() {
	Log::info(TAG, "Calendar app resumed");
	updateCalendarToday();
	return true;
}

void CalendarApp::onPause() {
	Log::info(TAG, "Calendar app paused");
}

void CalendarApp::onStop() {
	Log::info(TAG, "Calendar app stopped");
	m_container = nullptr;
	m_calendar = nullptr;
}

void CalendarApp::update() {
	// No periodic updates needed for now
}

void CalendarApp::createUI(void* parent) {
	auto* parentObj = static_cast<lv_obj_t*>(parent);

	// Create main container
	m_container = lv_obj_create(parentObj);
	lv_obj_set_size(m_container, LV_PCT(100), LV_PCT(100));
	lv_obj_set_style_pad_all(m_container, 0, 0);
	lv_obj_set_style_border_width(m_container, 0, 0);
	lv_obj_set_style_bg_opa(m_container, LV_OPA_TRANSP, 0);
	lv_obj_set_flex_flow(m_container, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(m_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	// Create calendar widget
	m_calendar = lv_calendar_create(m_container);
	lv_obj_set_size(m_calendar, LV_PCT(100), LV_PCT(100));
	lv_obj_set_style_radius(m_calendar, 0, 0);
	lv_obj_set_style_pad_all(m_calendar, 0, 0);
	lv_obj_set_style_border_width(m_calendar, 0, 0);

	// Remove padding from internal button matrix
	lv_obj_t* btnm = lv_calendar_get_btnmatrix(m_calendar);
	lv_obj_set_style_pad_all(btnm, 0, 0);
	lv_obj_set_style_pad_gap(btnm, 0, 0);

	// Add header with dropdown for easy month/year navigation
	lv_calendar_add_header_dropdown(m_calendar);

	// Set today's date
	updateCalendarToday();
}

void CalendarApp::updateCalendarToday() {
	if (!m_calendar) {
		return;
	}

	// Get current time
	time_t now = 0;
	struct tm timeinfo = {};
	time(&now);
	localtime_r(&now, &timeinfo);

	// Set today's date (tm_year is years since 1900, tm_mon is 0-11)
	lv_calendar_date_t today;
	today.year = timeinfo.tm_year + 1900;
	today.month = timeinfo.tm_mon + 1;
	today.day = timeinfo.tm_mday;

	lv_calendar_set_today_date(m_calendar, today.year, today.month, today.day);
	lv_calendar_set_month_shown(m_calendar, today.year, today.month);

	Log::info(TAG, "Calendar set to today: %d-%02d-%02d", today.year, today.month, today.day);
}

} // namespace System::Apps
