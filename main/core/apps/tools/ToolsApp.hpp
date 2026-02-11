#pragma once

#include "core/apps/AppManager.hpp"
#include "core/apps/settings/SettingsCommon.hpp"
#include "lvgl.h"
#include <functional>
#include <string>

#include "implementation/Calculator.hpp"
#include "implementation/DisplayTester.hpp"
#include "implementation/Flashlight.hpp"
#include "implementation/Stopwatch.hpp"

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

	// Tools
	Tools::Calculator m_calculator;
	Tools::Stopwatch m_stopwatch;
	Tools::Flashlight m_flashlight;
	Tools::DisplayTester m_displayTester;

	// Back callback
	std::function<void()> m_onBackToMain;

	// Navigation
	void showMainList();
	void hideAllViews();

	// Tool views
	void showCalculator();
	void showStopwatch();
	void showFlashlight();
	void showDisplayTester();
};

} // namespace System::Apps
