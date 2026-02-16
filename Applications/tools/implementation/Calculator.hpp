#pragma once

#include "lvgl.h"
#include <functional>
#include <string>

namespace System::Apps::Tools {

class Calculator {
public:

	Calculator() = default;
	~Calculator() = default;

	Calculator(const Calculator&) = delete;
	Calculator& operator=(const Calculator&) = delete;
	Calculator(Calculator&&) = delete;
	Calculator& operator=(Calculator&&) = delete;

	void createView(lv_obj_t* parent, std::function<void()> onBack);
	lv_obj_t* getView() const { return m_view; }
	void show();
	void hide();
	void destroy();

private:

	std::function<void()> m_onBack {};
	lv_obj_t* m_view {nullptr};
	lv_obj_t* m_calcDisplay {nullptr};
	lv_obj_t* m_calcExpressionLabel {nullptr};

	std::string m_calcInput {};
	std::string m_calcExpression {};
	std::string m_calcOperator {};
	double m_calcOperand {0};
	bool m_calcNewInput {true};

	void onCalcDigit(char digit);
	void onCalcOperator(const char* op);
	void onCalcEquals();
	void onCalcClear();
	void updateCalcDisplay();
};

} // namespace System::Apps::Tools
