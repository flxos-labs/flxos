#include "Calculator.hpp"
#include <cmath>
#include <cstdio>
#include <flx/ui/common/SettingsCommon.hpp>
#include <flx/ui/theming/layout_constants/LayoutConstants.hpp>
#include <flx/ui/theming/ui_constants/UiConstants.hpp>

using namespace flx::ui::common;

namespace System::Apps::Tools {

void Calculator::createView(lv_obj_t* parent, std::function<void()> onBack) {
	m_view = create_page_container(parent);

	lv_obj_t* backBtn = nullptr;
	create_header(m_view, "Calculator", &backBtn);

	m_onBack = onBack;
	add_back_button_event_cb(backBtn, &m_onBack);

	// Content area
	lv_obj_t* content = lv_obj_create(m_view);
	lv_obj_set_width(content, lv_pct(100));
	lv_obj_set_flex_grow(content, 1);
	lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_all(content, lv_dpx(UiConstants::PAD_DEFAULT), 0);
	lv_obj_set_style_pad_gap(content, lv_dpx(UiConstants::PAD_SMALL), 0);
	lv_obj_set_style_border_width(content, 0, 0);
	lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

	// Expression label (history)
	m_calcExpressionLabel = lv_label_create(content);
	lv_obj_set_width(m_calcExpressionLabel, lv_pct(100));
	lv_obj_set_style_text_align(m_calcExpressionLabel, LV_TEXT_ALIGN_RIGHT, 0);
	lv_obj_set_style_text_color(m_calcExpressionLabel, lv_color_hex(0xaaaaaa), 0); // Grey color for history
	lv_label_set_text(m_calcExpressionLabel, "");

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
            auto* calc = static_cast<Calculator*>(lv_event_get_user_data(e));
            auto* target = static_cast<lv_obj_t*>(lv_event_get_target(e));
            auto* btnText = static_cast<const char*>(lv_obj_get_user_data(target));

            if (btnText[0] >= '0' && btnText[0] <= '9') {
                calc->onCalcDigit(btnText[0]);
            } else if (btnText[0] == 'C') {
                calc->onCalcClear();
            } else if (btnText[0] == '=') {
                calc->onCalcEquals();
            } else {
                calc->onCalcOperator(btnText);
            } }, LV_EVENT_CLICKED, this);
	}
}

void Calculator::show() {
	if (m_view) lv_obj_remove_flag(m_view, LV_OBJ_FLAG_HIDDEN);
}

void Calculator::hide() {
	if (m_view) lv_obj_add_flag(m_view, LV_OBJ_FLAG_HIDDEN);
}

void Calculator::destroy() {
	if (m_view) {
		lv_obj_del(m_view);
		m_view = nullptr;
	}
	m_calcDisplay = nullptr;
	m_calcExpressionLabel = nullptr;
	onCalcClear();
}

void Calculator::onCalcDigit(char digit) {
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

void Calculator::onCalcOperator(const char* op) {
	if (!m_calcInput.empty()) {
		if (!m_calcOperator.empty() && !m_calcNewInput) {
			onCalcEquals();
		}
		m_calcOperand = std::stod(m_calcInput);
		m_calcOperator = op;
		m_calcExpression = m_calcInput + " " + op;
		m_calcNewInput = true;
		updateCalcDisplay();
	}
}

void Calculator::onCalcEquals() {
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
			m_calcExpression = "";
			updateCalcDisplay();
			// Reset to "0" after displaying error to prevent std::stod crash
			m_calcInput = "0";
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

	// Append second operand to expression
	if (m_calcExpression.find('=') == std::string::npos) {
		// Only append if it's not already showing a result
		m_calcExpression += " " + m_calcInput + " =";
	} else {
		// Let's keep it simple: just show result as new input start
		m_calcExpression = buf;
		m_calcExpression += " =";
	}

	m_calcInput = buf;
	m_calcOperator.clear();
	m_calcNewInput = true;
	updateCalcDisplay();
}

void Calculator::onCalcClear() {
	m_calcInput.clear();
	m_calcExpression.clear();
	m_calcOperator.clear();
	m_calcOperand = 0;
	m_calcNewInput = true;
	if (m_calcDisplay) {
		lv_label_set_text(m_calcDisplay, "0");
	}
	if (m_calcExpressionLabel) {
		lv_label_set_text(m_calcExpressionLabel, "");
	}
}

void Calculator::updateCalcDisplay() {
	if (m_calcDisplay) {
		lv_label_set_text(m_calcDisplay, m_calcInput.empty() ? "0" : m_calcInput.c_str());
	}
	if (m_calcExpressionLabel) {
		lv_label_set_text(m_calcExpressionLabel, m_calcExpression.c_str());
	}
}

} // namespace System::Apps::Tools
