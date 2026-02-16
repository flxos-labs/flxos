#pragma once

#include "lvgl.h"
#include <flx/apps/App.hpp>
#include <flx/apps/AppManifest.hpp>
#include <flx/ui/common/SettingsCommon.hpp>
#include <functional>
#include <string>

#include "implementation/Calculator.hpp"
#include "implementation/DisplayTester.hpp"
#include "implementation/Flashlight.hpp"
#include "implementation/Screenshot.hpp"
#include "implementation/Stopwatch.hpp"

namespace System::Apps {

class ToolsApp : public flx::apps::App {
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

	static const flx::apps::AppManifest manifest;

private:

	// Main container and list
	lv_obj_t* m_container {nullptr};
	lv_obj_t* m_mainList {nullptr};

	// Tools
	Tools::Calculator m_calculator;
	Tools::Stopwatch m_stopwatch;
	Tools::Flashlight m_flashlight;
	Tools::DisplayTester m_displayTester;
	Tools::Screenshot m_screenshot;

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
	void showScreenshot();
};

} // namespace System::Apps
