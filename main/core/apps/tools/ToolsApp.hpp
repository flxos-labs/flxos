#pragma once

#include "core/apps/AppManager.hpp"
#include "core/apps/settings/SettingsCommon.hpp"
#include "lvgl.h"
#include <functional>

namespace System::Apps {

class ToolsApp : public App {
public:

	ToolsApp() = default;
	~ToolsApp() override = default;

	bool onStart() override;
	bool onResume() override;
	void onPause() override;
	void onStop() override;
	void createUI(void* parent) override;
	void update() override;

	std::string getPackageName() const override { return "com.flxos.tools"; }
	std::string getAppName() const override { return "Tools"; }
	const void* getIcon() const override { return LV_SYMBOL_LIST; }

private:

	// Main container and list
	lv_obj_t* m_container {nullptr};
	lv_obj_t* m_mainList {nullptr};

	// Tool views
	lv_obj_t* m_calcView {nullptr};
	lv_obj_t* m_stopwatchView {nullptr};
	lv_obj_t* m_flashlightView {nullptr};
	lv_obj_t* m_rgbView {nullptr};

	// Calculator state
	lv_obj_t* m_calcDisplay {nullptr};
	std::string m_calcInput {};
	std::string m_calcOperator {};
	double m_calcOperand {0};
	bool m_calcNewInput {true};

	// Stopwatch state
	lv_obj_t* m_stopwatchLabel {nullptr};
	lv_obj_t* m_stopwatchStartBtn {nullptr};
	uint32_t m_stopwatchStartTime {0};
	uint32_t m_stopwatchElapsed {0};
	bool m_stopwatchRunning {false};

	// Flashlight state
	lv_obj_t* m_flashlightContainer {nullptr};
	bool m_flashlightOn {false};

	// RGB state
	lv_obj_t* m_rgbDisplay {nullptr};

	// Back callback
	std::function<void()> m_onBackToMain;

	// Navigation
	void showMainList();
	void hideAllViews();

	// Tool views
	void showCalculator();
	void showStopwatch();
	void showFlashlight();
	void showRGBTester();

	// Tool UI creators
	void createCalculatorView();
	void createStopwatchView();
	void createFlashlightView();
	void createRGBTesterView();

	// Calculator helpers
	void onCalcDigit(char digit);
	void onCalcOperator(const char* op);
	void onCalcEquals();
	void onCalcClear();
	void updateCalcDisplay();

	// Stopwatch helpers
	void updateStopwatchDisplay();
};

} // namespace System::Apps
