#include "ToolsApp.hpp"
#include "../../ui/theming/layout_constants/LayoutConstants.hpp"
#include "../../ui/theming/ui_constants/UiConstants.hpp"
#include "core/common/Logger.hpp"
#include "esp_timer.h"
#include <cmath>
#include <cstdio>

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
	if (m_stopwatchRunning) {
		m_stopwatchElapsed += (esp_timer_get_time() / 1000) - m_stopwatchStartTime;
		m_stopwatchRunning = false;
	}
}

void ToolsApp::onStop() {
	Log::info(TAG, "Tools app stopped");
	m_container = nullptr;
	m_mainList = nullptr;
	m_calcView = nullptr;
	m_stopwatchView = nullptr;
	m_flashlightView = nullptr;
	m_rgbView = nullptr;
	m_calcDisplay = nullptr;
	m_stopwatchLabel = nullptr;
	m_stopwatchStartBtn = nullptr;
	m_flashlightContainer = nullptr;
	m_rgbDisplay = nullptr;

	m_calcInput.clear();
	m_calcOperator.clear();
	m_calcOperand = 0;
	m_calcNewInput = true;
	m_stopwatchElapsed = 0;
	m_stopwatchRunning = false;
	m_flashlightOn = false;
}

void ToolsApp::createUI(void* parent) {
	m_container = static_cast<lv_obj_t*>(parent);
	m_onBackToMain = [this]() { showMainList(); };
	showMainList();
}

void ToolsApp::update() {
	if (!isActive()) return;

	if (m_stopwatchRunning && m_stopwatchLabel) {
		updateStopwatchDisplay();
	}
}

// ============================================================================
// Navigation
// ============================================================================

void ToolsApp::hideAllViews() {
	if (m_mainList) lv_obj_add_flag(m_mainList, LV_OBJ_FLAG_HIDDEN);
	if (m_calcView) lv_obj_add_flag(m_calcView, LV_OBJ_FLAG_HIDDEN);
	if (m_stopwatchView) lv_obj_add_flag(m_stopwatchView, LV_OBJ_FLAG_HIDDEN);
	if (m_flashlightView) lv_obj_add_flag(m_flashlightView, LV_OBJ_FLAG_HIDDEN);
	if (m_rgbView) lv_obj_add_flag(m_rgbView, LV_OBJ_FLAG_HIDDEN);
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

		lv_obj_t* rgbBtn = Settings::add_list_btn(m_mainList, LV_SYMBOL_IMAGE, "RGB Tester");
		lv_obj_add_event_cb(rgbBtn, [](lv_event_t* e) {
			auto* app = static_cast<ToolsApp*>(lv_event_get_user_data(e));
			app->showRGBTester(); }, LV_EVENT_CLICKED, this);
	} else {
		lv_obj_remove_flag(m_mainList, LV_OBJ_FLAG_HIDDEN);
	}
}

void ToolsApp::showCalculator() {
	hideAllViews();
	if (m_calcView == nullptr) {
		createCalculatorView();
	}
	lv_obj_remove_flag(m_calcView, LV_OBJ_FLAG_HIDDEN);
}

void ToolsApp::showStopwatch() {
	hideAllViews();
	if (m_stopwatchView == nullptr) {
		createStopwatchView();
	}
	lv_obj_remove_flag(m_stopwatchView, LV_OBJ_FLAG_HIDDEN);
}

void ToolsApp::showFlashlight() {
	hideAllViews();
	if (m_flashlightView == nullptr) {
		createFlashlightView();
	}
	lv_obj_remove_flag(m_flashlightView, LV_OBJ_FLAG_HIDDEN);
}

void ToolsApp::showRGBTester() {
	hideAllViews();
	if (m_rgbView == nullptr) {
		createRGBTesterView();
	}
	lv_obj_remove_flag(m_rgbView, LV_OBJ_FLAG_HIDDEN);
}

// ============================================================================
// Calculator
// ============================================================================

void ToolsApp::createCalculatorView() {
	m_calcView = Settings::create_page_container(m_container);

	lv_obj_t* backBtn = nullptr;
	Settings::create_header(m_calcView, "Calculator", &backBtn);
	Settings::add_back_button_event_cb(backBtn, &m_onBackToMain);

	// Content area
	lv_obj_t* content = lv_obj_create(m_calcView);
	lv_obj_set_width(content, lv_pct(100));
	lv_obj_set_flex_grow(content, 1);
	lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_all(content, lv_dpx(UiConstants::PAD_DEFAULT), 0);
	lv_obj_set_style_pad_gap(content, lv_dpx(UiConstants::PAD_SMALL), 0);
	lv_obj_set_style_border_width(content, 0, 0);
	lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

	// Display
	m_calcDisplay = lv_label_create(content);
	lv_obj_set_width(m_calcDisplay, lv_pct(100));
	lv_obj_set_style_pad_all(m_calcDisplay, lv_dpx(UiConstants::PAD_LARGE), 0);
	lv_obj_set_style_text_align(m_calcDisplay, LV_TEXT_ALIGN_RIGHT, 0);
	lv_obj_set_style_radius(m_calcDisplay, lv_dpx(UiConstants::RADIUS_SMALL), 0);
	lv_obj_set_style_border_width(m_calcDisplay, 1, 0);
	lv_label_set_text(m_calcDisplay, "0");

	// Button grid
	static const char* buttons[] = {
		"7", "8", "9", "/",
		"4", "5", "6", "*",
		"1", "2", "3", "-",
		"C", "0", "=", "+"
	};

	lv_obj_t* grid = lv_obj_create(content);
	lv_obj_set_width(grid, lv_pct(100));
	lv_obj_set_flex_grow(grid, 1);
	lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
	lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_all(grid, 0, 0);
	lv_obj_set_style_bg_opa(grid, 0, 0);
	lv_obj_set_style_border_width(grid, 0, 0);
	lv_obj_set_style_pad_gap(grid, lv_dpx(UiConstants::PAD_SMALL), 0);

	for (int i = 0; i < 16; i++) {
		lv_obj_t* btn = lv_button_create(grid);
		lv_obj_set_size(btn, lv_pct(22), lv_pct(22));

		lv_obj_t* label = lv_label_create(btn);
		lv_label_set_text(label, buttons[i]);
		lv_obj_center(label);

		lv_obj_set_user_data(btn, (void*)buttons[i]);

		// Style operators differently
		if (i % 4 == 3 || buttons[i][0] == 'C' || buttons[i][0] == '=') {
			lv_obj_set_style_bg_color(btn, lv_color_hex(0x4a4e69), 0);
		}

		lv_obj_add_event_cb(btn, [](lv_event_t* e) {
			auto* app = static_cast<ToolsApp*>(lv_event_get_user_data(e));
			auto* target = static_cast<lv_obj_t*>(lv_event_get_target(e));
			auto* btnText = static_cast<const char*>(lv_obj_get_user_data(target));

			if (btnText[0] >= '0' && btnText[0] <= '9') {
				app->onCalcDigit(btnText[0]);
			} else if (btnText[0] == 'C') {
				app->onCalcClear();
			} else if (btnText[0] == '=') {
				app->onCalcEquals();
			} else {
				app->onCalcOperator(btnText);
			} }, LV_EVENT_CLICKED, this);
	}
}

void ToolsApp::onCalcDigit(char digit) {
	if (m_calcNewInput) {
		m_calcInput = digit;
		m_calcNewInput = false;
	} else {
		if (m_calcInput.length() < 12) {
			m_calcInput += digit;
		}
	}
	updateCalcDisplay();
}

void ToolsApp::onCalcOperator(const char* op) {
	if (!m_calcInput.empty()) {
		if (!m_calcOperator.empty() && !m_calcNewInput) {
			onCalcEquals();
		}
		m_calcOperand = std::stod(m_calcInput);
		m_calcOperator = op;
		m_calcNewInput = true;
	}
}

void ToolsApp::onCalcEquals() {
	if (m_calcOperator.empty() || m_calcInput.empty()) return;

	double b = std::stod(m_calcInput);
	double result = 0;

	if (m_calcOperator == "+") result = m_calcOperand + b;
	else if (m_calcOperator == "-")
		result = m_calcOperand - b;
	else if (m_calcOperator == "*")
		result = m_calcOperand * b;
	else if (m_calcOperator == "/") {
		if (b != 0) result = m_calcOperand / b;
		else {
			m_calcInput = "Error";
			updateCalcDisplay();
			m_calcNewInput = true;
			m_calcOperator.clear();
			return;
		}
	}

	char buf[24];
	if (std::floor(result) == result && std::abs(result) < 1e9) {
		snprintf(buf, sizeof(buf), "%.0f", result);
	} else {
		snprintf(buf, sizeof(buf), "%.6g", result);
	}
	m_calcInput = buf;
	m_calcOperator.clear();
	m_calcNewInput = true;
	updateCalcDisplay();
}

void ToolsApp::onCalcClear() {
	m_calcInput.clear();
	m_calcOperator.clear();
	m_calcOperand = 0;
	m_calcNewInput = true;
	if (m_calcDisplay) {
		lv_label_set_text(m_calcDisplay, "0");
	}
}

void ToolsApp::updateCalcDisplay() {
	if (m_calcDisplay) {
		lv_label_set_text(m_calcDisplay, m_calcInput.empty() ? "0" : m_calcInput.c_str());
	}
}

// ============================================================================
// Stopwatch
// ============================================================================

void ToolsApp::createStopwatchView() {
	m_stopwatchView = Settings::create_page_container(m_container);

	lv_obj_t* backBtn = nullptr;
	Settings::create_header(m_stopwatchView, "Stopwatch", &backBtn);
	Settings::add_back_button_event_cb(backBtn, &m_onBackToMain);

	// Content
	lv_obj_t* content = lv_obj_create(m_stopwatchView);
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
		auto* app = static_cast<ToolsApp*>(lv_event_get_user_data(e));
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
		auto* app = static_cast<ToolsApp*>(lv_event_get_user_data(e));
		app->m_stopwatchRunning = false;
		app->m_stopwatchElapsed = 0;
		app->m_stopwatchStartTime = 0;
		lv_label_set_text(app->m_stopwatchLabel, "00:00.00");
		lv_label_set_text(lv_obj_get_child(app->m_stopwatchStartBtn, 0), LV_SYMBOL_PLAY " Start"); }, LV_EVENT_CLICKED, this);
}

void ToolsApp::updateStopwatchDisplay() {
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

// ============================================================================
// Flashlight
// ============================================================================

void ToolsApp::createFlashlightView() {
	m_flashlightView = Settings::create_page_container(m_container);

	lv_obj_t* backBtn = nullptr;
	Settings::create_header(m_flashlightView, "Flashlight", &backBtn);
	Settings::add_back_button_event_cb(backBtn, &m_onBackToMain);

	m_flashlightContainer = lv_obj_create(m_flashlightView);
	lv_obj_set_size(m_flashlightContainer, lv_pct(100), lv_pct(100));
	lv_obj_set_flex_grow(m_flashlightContainer, 1);
	lv_obj_set_style_bg_color(m_flashlightContainer, lv_color_hex(0x2d2d2d), 0);
	lv_obj_set_style_border_width(m_flashlightContainer, 0, 0);
	lv_obj_set_flex_flow(m_flashlightContainer, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(m_flashlightContainer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_t* icon = lv_label_create(m_flashlightContainer);
	lv_label_set_text(icon, LV_SYMBOL_EYE_OPEN);

	lv_obj_t* hint = lv_label_create(m_flashlightContainer);
	lv_label_set_text(hint, "Tap to toggle");
	lv_obj_set_style_margin_top(hint, lv_dpx(UiConstants::PAD_LARGE), 0);

	lv_obj_add_event_cb(m_flashlightContainer, [](lv_event_t* e) {
		auto* app = static_cast<ToolsApp*>(lv_event_get_user_data(e));
		app->m_flashlightOn = !app->m_flashlightOn;

		if (app->m_flashlightOn) {
			lv_obj_set_style_bg_color(app->m_flashlightContainer, lv_color_white(), 0);
			lv_obj_set_style_text_color(lv_obj_get_child(app->m_flashlightContainer, 0), lv_color_black(), 0);
			lv_obj_set_style_text_color(lv_obj_get_child(app->m_flashlightContainer, 1), lv_color_black(), 0);
		} else {
			lv_obj_set_style_bg_color(app->m_flashlightContainer, lv_color_hex(0x2d2d2d), 0);
			lv_obj_set_style_text_color(lv_obj_get_child(app->m_flashlightContainer, 0), lv_color_white(), 0);
			lv_obj_set_style_text_color(lv_obj_get_child(app->m_flashlightContainer, 1), lv_color_white(), 0);
		} }, LV_EVENT_CLICKED, this);
}

// ============================================================================
// RGB Tester
// ============================================================================

void ToolsApp::createRGBTesterView() {
	m_rgbView = Settings::create_page_container(m_container);

	lv_obj_t* backBtn = nullptr;
	Settings::create_header(m_rgbView, "RGB Tester", &backBtn);
	Settings::add_back_button_event_cb(backBtn, &m_onBackToMain);

	// Color display
	m_rgbDisplay = lv_obj_create(m_rgbView);
	lv_obj_set_width(m_rgbDisplay, lv_pct(100));
	lv_obj_set_flex_grow(m_rgbDisplay, 1);
	lv_obj_set_style_bg_color(m_rgbDisplay, lv_color_hex(0xFF0000), 0);
	lv_obj_set_style_border_width(m_rgbDisplay, 0, 0);
	lv_obj_set_flex_flow(m_rgbDisplay, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(m_rgbDisplay, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_clear_flag(m_rgbDisplay, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t* colorLabel = lv_label_create(m_rgbDisplay);
	lv_label_set_text(colorLabel, "RED");
	lv_obj_set_style_text_color(colorLabel, lv_color_white(), 0);

	// Color buttons
	lv_obj_t* btnRow = lv_obj_create(m_rgbView);
	lv_obj_set_size(btnRow, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(btnRow, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(btnRow, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_bg_opa(btnRow, 0, 0);
	lv_obj_set_style_border_width(btnRow, 0, 0);
	lv_obj_set_style_pad_all(btnRow, lv_dpx(UiConstants::PAD_DEFAULT), 0);

	struct ColorDef {
		uint32_t color;
		const char* name;
		lv_color_t textColor;
	};

	static const ColorDef colors[] = {
		{0xFF0000, "RED", lv_color_white()},
		{0x00FF00, "GREEN", lv_color_black()},
		{0x0000FF, "BLUE", lv_color_white()},
		{0xFFFFFF, "WHITE", lv_color_black()},
		{0x000000, "BLACK", lv_color_white()}
	};

	for (int i = 0; i < 5; i++) {
		lv_obj_t* btn = lv_button_create(btnRow);
		lv_obj_set_size(btn, lv_pct(18), lv_dpx(LayoutConstants::SIZE_TOUCH_TARGET));
		lv_obj_set_style_bg_color(btn, lv_color_hex(colors[i].color), 0);
		if (colors[i].color == 0x000000) {
			lv_obj_set_style_border_color(btn, lv_color_white(), 0);
			lv_obj_set_style_border_width(btn, UiConstants::BORDER_THIN, 0);
		}

		lv_obj_set_user_data(btn, (void*)(uintptr_t)i);

		lv_obj_add_event_cb(btn, [](lv_event_t* e) {
			auto* app = static_cast<ToolsApp*>(lv_event_get_user_data(e));
			auto* target = static_cast<lv_obj_t*>(lv_event_get_target(e));
			int idx = (int)(uintptr_t)lv_obj_get_user_data(target);

			static const ColorDef colors[] = {
				{0xFF0000, "RED", lv_color_white()},
				{0x00FF00, "GREEN", lv_color_black()},
				{0x0000FF, "BLUE", lv_color_white()},
				{0xFFFFFF, "WHITE", lv_color_black()},
				{0x000000, "BLACK", lv_color_white()}
			};

			lv_obj_set_style_bg_color(app->m_rgbDisplay, lv_color_hex(colors[idx].color), 0);
			lv_obj_t* label = lv_obj_get_child(app->m_rgbDisplay, 0);
			lv_label_set_text(label, colors[idx].name);
			lv_obj_set_style_text_color(label, colors[idx].textColor, 0); }, LV_EVENT_CLICKED, this);
	}
}

} // namespace System::Apps
