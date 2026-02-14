#include "ToolsApp.hpp"
#include "../../ui/theming/layout_constants/LayoutConstants.hpp"
#include "../../ui/theming/ui_constants/UiConstants.hpp"
#include <flx/core/Logger.hpp>

static constexpr std::string_view TAG = "ToolsApp";

namespace System::Apps {

bool ToolsApp::onStart() {
	Log::info(TAG, "Tools app started");
	return true;
}

bool ToolsApp::onResume() {
	Log::info(TAG, "Tools app resumed");
	return true;
}

void ToolsApp::onPause() {
	Log::info(TAG, "Tools app paused");
	m_stopwatch.onPause();
}

void ToolsApp::onStop() {
	Log::info(TAG, "Tools app stopped");
	m_container = nullptr;
	m_mainList = nullptr;

	m_calculator.destroy();
	m_stopwatch.destroy();
	m_flashlight.destroy();
	m_displayTester.destroy();
	m_screenshot.destroy();
}

void ToolsApp::createUI(void* parent) {
	m_container = static_cast<lv_obj_t*>(parent);
	m_onBackToMain = [this]() { showMainList(); };
	showMainList();
}

void ToolsApp::update() {
	if (!isActive()) return;
	m_stopwatch.update();
}

// ============================================================================
// Navigation
// ============================================================================

void ToolsApp::hideAllViews() {
	if (m_mainList) lv_obj_add_flag(m_mainList, LV_OBJ_FLAG_HIDDEN);
	m_calculator.hide();
	m_stopwatch.hide();
	m_flashlight.hide();
	m_displayTester.hide();
	m_screenshot.hide();
}

void ToolsApp::showMainList() {
	hideAllViews();

	if (m_mainList == nullptr) {
		m_mainList = lv_list_create(m_container);
		lv_obj_set_size(m_mainList, lv_pct(100), lv_pct(100));
		lv_obj_set_style_border_width(m_mainList, 0, 0);

		lv_list_add_text(m_mainList, "Utilities");

		lv_obj_t* calcBtn = Settings::add_list_btn(m_mainList, LV_SYMBOL_CHARGE, "Calculator");
		lv_obj_add_event_cb(calcBtn, [](lv_event_t* e) {
			auto* app = static_cast<ToolsApp*>(lv_event_get_user_data(e));
			app->showCalculator(); }, LV_EVENT_CLICKED, this);

		lv_obj_t* stopwatchBtn = Settings::add_list_btn(m_mainList, LV_SYMBOL_PLAY, "Stopwatch");
		lv_obj_add_event_cb(stopwatchBtn, [](lv_event_t* e) {
			auto* app = static_cast<ToolsApp*>(lv_event_get_user_data(e));
			app->showStopwatch(); }, LV_EVENT_CLICKED, this);

		lv_list_add_text(m_mainList, "Display Tools");

		lv_obj_t* flashBtn = Settings::add_list_btn(m_mainList, LV_SYMBOL_EYE_OPEN, "Flashlight");
		lv_obj_add_event_cb(flashBtn, [](lv_event_t* e) {
			auto* app = static_cast<ToolsApp*>(lv_event_get_user_data(e));
			app->showFlashlight(); }, LV_EVENT_CLICKED, this);

		lv_obj_t* rgbBtn = Settings::add_list_btn(m_mainList, LV_SYMBOL_IMAGE, "Display Tester");
		lv_obj_add_event_cb(rgbBtn, [](lv_event_t* e) {
			auto* app = static_cast<ToolsApp*>(lv_event_get_user_data(e));
			app->showDisplayTester(); }, LV_EVENT_CLICKED, this);

		lv_obj_t* screenshotBtn = Settings::add_list_btn(m_mainList, LV_SYMBOL_IMAGE, "Screenshot");
		lv_obj_add_event_cb(screenshotBtn, [](lv_event_t* e) {
			auto* app = static_cast<ToolsApp*>(lv_event_get_user_data(e));
			app->showScreenshot(); }, LV_EVENT_CLICKED, this);
	} else {
		lv_obj_remove_flag(m_mainList, LV_OBJ_FLAG_HIDDEN);
	}
}

void ToolsApp::showCalculator() {
	hideAllViews();
	if (m_calculator.getView() == nullptr) {
		m_calculator.createView(m_container, m_onBackToMain);
	}
	m_calculator.show();
}

void ToolsApp::showStopwatch() {
	hideAllViews();
	if (m_stopwatch.getView() == nullptr) {
		m_stopwatch.createView(m_container, m_onBackToMain);
	}
	m_stopwatch.show();
}

void ToolsApp::showFlashlight() {
	hideAllViews();
	if (m_flashlight.getView() == nullptr) {
		m_flashlight.createView(m_container, m_onBackToMain);
	}
	m_flashlight.show();
}

void ToolsApp::showDisplayTester() {
	hideAllViews();
	if (m_displayTester.getView() == nullptr) {
		m_displayTester.createView(m_container, m_onBackToMain);
	}
	m_displayTester.show();
}

void ToolsApp::showScreenshot() {
	hideAllViews();
	if (m_screenshot.getView() == nullptr) {
		m_screenshot.createView(m_container, m_onBackToMain);
	}
	m_screenshot.show();
}

} // namespace System::Apps
