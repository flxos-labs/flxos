#include "Stopwatch.hpp"
#include "../../ui/theming/layout_constants/LayoutConstants.hpp"
#include "../../ui/theming/ui_constants/UiConstants.hpp"
#include "core/apps/settings/SettingsCommon.hpp"
#include "esp_timer.h"

namespace System::Apps::Tools {

void Stopwatch::createView(lv_obj_t* parent, std::function<void()> onBack) {
	m_view = Settings::create_page_container(parent);

	lv_obj_t* backBtn = nullptr;
	Settings::create_header(m_view, "Stopwatch", &backBtn);

	lv_obj_add_event_cb(backBtn, [](lv_event_t* e) {
        auto* fn = static_cast<std::function<void()>*>(lv_event_get_user_data(e));
        if (fn && *fn) (*fn)(); }, LV_EVENT_CLICKED, new std::function<void()>(onBack));

	// Content
	lv_obj_t* content = lv_obj_create(m_view);
	lv_obj_set_size(content, lv_pct(100), lv_pct(100));
	lv_obj_set_flex_grow(content, 1);
	lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_border_width(content, 0, 0);
	lv_obj_set_style_pad_all(content, lv_dpx(UiConstants::PAD_LARGE), 0);

	// Time display
	m_stopwatchLabel = lv_label_create(content);
	lv_label_set_text(m_stopwatchLabel, "00:00.00");

	// Buttons
	lv_obj_t* btnRow = lv_obj_create(content);
	lv_obj_set_size(btnRow, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(btnRow, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(btnRow, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_gap(btnRow, lv_dpx(UiConstants::PAD_DEFAULT), 0);
	lv_obj_set_style_bg_opa(btnRow, 0, 0);
	lv_obj_set_style_border_width(btnRow, 0, 0);
	lv_obj_set_style_margin_top(btnRow, lv_dpx(LayoutConstants::MARGIN_SECTION), 0);

	m_stopwatchStartBtn = lv_button_create(btnRow);
	lv_obj_set_size(m_stopwatchStartBtn, lv_pct(40), lv_dpx(LayoutConstants::SIZE_TOUCH_TARGET));
	lv_obj_t* startLabel = lv_label_create(m_stopwatchStartBtn);
	lv_label_set_text(startLabel, LV_SYMBOL_PLAY " Start");
	lv_obj_center(startLabel);

	lv_obj_add_event_cb(m_stopwatchStartBtn, [](lv_event_t* e) {
        auto* app = static_cast<Stopwatch*>(lv_event_get_user_data(e));
        if (app->m_stopwatchRunning) {
            app->m_stopwatchElapsed += (esp_timer_get_time() / 1000) - app->m_stopwatchStartTime;
            app->m_stopwatchRunning = false;
            lv_label_set_text(lv_obj_get_child(app->m_stopwatchStartBtn, 0), LV_SYMBOL_PLAY " Start");
        } else {
            app->m_stopwatchStartTime = esp_timer_get_time() / 1000;
            app->m_stopwatchRunning = true;
            lv_label_set_text(lv_obj_get_child(app->m_stopwatchStartBtn, 0), LV_SYMBOL_PAUSE " Stop");
        } }, LV_EVENT_CLICKED, this);

	lv_obj_t* resetBtn = lv_button_create(btnRow);
	lv_obj_set_size(resetBtn, lv_pct(40), lv_dpx(LayoutConstants::SIZE_TOUCH_TARGET));
	lv_obj_t* resetLabel = lv_label_create(resetBtn);
	lv_label_set_text(resetLabel, LV_SYMBOL_REFRESH " Reset");
	lv_obj_center(resetLabel);

	lv_obj_add_event_cb(resetBtn, [](lv_event_t* e) {
        auto* app = static_cast<Stopwatch*>(lv_event_get_user_data(e));
        app->m_stopwatchRunning = false;
        app->m_stopwatchElapsed = 0;
        app->m_stopwatchStartTime = 0;
        lv_label_set_text(app->m_stopwatchLabel, "00:00.00");
        lv_label_set_text(lv_obj_get_child(app->m_stopwatchStartBtn, 0), LV_SYMBOL_PLAY " Start"); }, LV_EVENT_CLICKED, this);
}

void Stopwatch::update() {
	if (m_stopwatchRunning && m_stopwatchLabel) {
		updateStopwatchDisplay();
	}
}

void Stopwatch::onPause() {
	if (m_stopwatchRunning) {
		m_stopwatchElapsed += (esp_timer_get_time() / 1000) - m_stopwatchStartTime;
		m_stopwatchRunning = false;
		// Also update UI to show it's paused? The original code didn't seem to explicitly update UI on pause,
		// just stopped the timer logic. But the button text would still say "Stop".
		// The resumption logic in ToolsApp::onResume just said "Tools app resumed".
		// If we pause, we should probably update the button text if the user comes back.
		// But the original code only did this in onPause:
		// m_stopwatchElapsed += (esp_timer_get_time() / 1000) - m_stopwatchStartTime;
		// m_stopwatchRunning = false;

		// If we resume, m_stopwatchRunning is false, so update() won't update display.
		// And the button will still say "Stop" unless we change it.
		if (m_stopwatchStartBtn) {
			lv_label_set_text(lv_obj_get_child(m_stopwatchStartBtn, 0), LV_SYMBOL_PLAY " Start");
		}
	}
}

void Stopwatch::onStop() {
	m_stopwatchElapsed = 0;
	m_stopwatchRunning = false;
	m_stopwatchLabel = nullptr;
	m_stopwatchStartBtn = nullptr;
}

void Stopwatch::show() {
	if (m_view) lv_obj_remove_flag(m_view, LV_OBJ_FLAG_HIDDEN);
}

void Stopwatch::hide() {
	if (m_view) lv_obj_add_flag(m_view, LV_OBJ_FLAG_HIDDEN);
}

void Stopwatch::destroy() {
	if (m_view) {
		lv_obj_del(m_view);
		m_view = nullptr;
	}
	onStop();
}

void Stopwatch::updateStopwatchDisplay() {
	if (!m_stopwatchLabel) return;

	uint32_t totalMs = m_stopwatchElapsed;
	if (m_stopwatchRunning) {
		totalMs += (esp_timer_get_time() / 1000) - m_stopwatchStartTime;
	}

	uint32_t minutes = totalMs / 60000;
	uint32_t seconds = (totalMs / 1000) % 60;
	uint32_t centiseconds = (totalMs / 10) % 100;

	lv_label_set_text_fmt(m_stopwatchLabel, "%02lu:%02lu.%02lu", minutes, seconds, centiseconds);
}

} // namespace System::Apps::Tools
