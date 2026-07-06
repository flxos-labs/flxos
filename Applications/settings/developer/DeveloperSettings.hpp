#pragma once

#include <flx/apps/AppManager.hpp>
#include <flx/apps/Intent.hpp>
#include <flx/core/Logger.hpp>
#include <flx/core/Observable.hpp>
#include <flx/system/SystemDiagnostics.hpp>
#include <flx/system/managers/SettingsManager.hpp>
#include <flx/ui/LvglObserverBridge.hpp>
#include <lvgl.h>
#include <memory>

#include "settings/SettingsPageBase.hpp"
#include "DemoModeService.hpp"

using namespace flx::ui::common;
using namespace flx::system;

namespace System::Apps::Settings {

class DeveloperSettings : public SettingsPageBase {
public:

	using SettingsPageBase::SettingsPageBase;

protected:

	void createUI() override {
		auto& sm = SettingsManager::getInstance();
		static bool registered = false;
		if (!registered) {
			sm.registerSetting("developer.demo_mode", DemoModeService::demoMode);
			sm.registerSetting("developer.demo_simulate_battery", DemoModeService::simulateBattery);
			sm.registerSetting("developer.demo_simulate_wifi", DemoModeService::simulateWifi);
			sm.registerSetting("developer.demo_simulate_bluetooth", DemoModeService::simulateBluetooth);
			sm.registerSetting("developer.demo_simulate_hotspot", DemoModeService::simulateHotspot);
			sm.registerSetting("developer.demo_simulate_notifications", DemoModeService::simulateNotifications);
			sm.registerSetting("developer.demo_interval_ms", DemoModeService::demoIntervalMs);
			registered = true;
		}

		DemoModeService::init();

		m_verboseLoggingBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(sm.getVerboseLogging());
		m_diagnosticOverlayBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(sm.getDiagnosticOverlay());
		m_demoModeBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(DemoModeService::demoMode);
		m_simulateBatteryBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(DemoModeService::simulateBattery);
		m_simulateWifiBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(DemoModeService::simulateWifi);
		m_simulateBluetoothBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(DemoModeService::simulateBluetooth);
		m_simulateHotspotBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(DemoModeService::simulateHotspot);
		m_simulateNotificationsBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(DemoModeService::simulateNotifications);
		m_demoIntervalBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(DemoModeService::demoIntervalMs);

		m_container = create_page_container(m_parent);

		lv_obj_t* backBtn = nullptr;
		create_header(m_container, "Developer Options", &backBtn);
		add_back_button_event_cb(backBtn, &m_onBack);

		m_list = create_settings_list(m_container);

		// ── Logging Section ──
		lv_list_add_text(m_list, "Debugging & Logs");

		lv_obj_t* verboseBtn = add_list_btn(m_list, LV_SYMBOL_LIST, "Verbose Logging");
		lv_obj_set_flex_grow(lv_obj_get_child(verboseBtn, 1), 1);
		lv_obj_t* verboseSw = lv_switch_create(verboseBtn);
		lv_obj_add_flag(verboseSw, LV_OBJ_FLAG_EVENT_BUBBLE);
		lv_obj_bind_checked(verboseSw, m_verboseLoggingBridge->getSubject());

		// ── Diagnostic Overlay Section ──
		lv_list_add_text(m_list, "Performance & Overlays");

		lv_obj_t* overlayBtn = add_list_btn(m_list, LV_SYMBOL_EYE_OPEN, "Diagnostic Overlay");
		lv_obj_set_flex_grow(lv_obj_get_child(overlayBtn, 1), 1);
		lv_obj_t* overlaySw = lv_switch_create(overlayBtn);
		lv_obj_add_flag(overlaySw, LV_OBJ_FLAG_EVENT_BUBBLE);
		lv_obj_bind_checked(overlaySw, m_diagnosticOverlayBridge->getSubject());

		// ── UI Simulation / Demo ──
		lv_list_add_text(m_list, "UI Simulation / Demo");

		lv_obj_t* demoPageBtn = lv_list_add_button(m_list, LV_SYMBOL_PLAY, "Configure UI Demo Mode");
		lv_obj_add_event_cb(
			demoPageBtn,
			[](lv_event_t* e) {
				auto* instance = (DeveloperSettings*)lv_event_get_user_data(e);
				instance->showDemoPage();
			},
			LV_EVENT_CLICKED, this);

		// ── Actions Section ──
		lv_list_add_text(m_list, "Diagnostic Actions");

		lv_obj_t* dumpDiagnosticsBtn = lv_list_add_button(m_list, LV_SYMBOL_DOWNLOAD, "Dump Diagnostics to Console");
		lv_obj_add_event_cb(
			dumpDiagnosticsBtn,
			[](lv_event_t* /*e*/) {
				flx::system::dumpSystemDiagnostics();
			},
			LV_EVENT_CLICKED, nullptr);

		lv_obj_t* viewSystemInfoBtn = lv_list_add_button(m_list, LV_SYMBOL_SETTINGS, "View Detailed System Info");
		lv_obj_add_event_cb(
			viewSystemInfoBtn,
			[](lv_event_t* /*e*/) {
				flx::apps::Intent intent = flx::apps::Intent::forApp("com.flxos.systeminfo");
				flx::apps::AppManager::getInstance().startApp(intent);
			},
			LV_EVENT_CLICKED, nullptr);
	}

	void onDestroy() override {
		DemoModeService::stop();
		DemoModeService::demoMode.set(0);

		if (m_demoContainer && lv_obj_is_valid(m_demoContainer)) {
			lv_obj_delete(m_demoContainer);
		}
		m_demoContainer = nullptr;
		m_demoList = nullptr;

		m_verboseLoggingBridge.reset();
		m_diagnosticOverlayBridge.reset();
		m_demoModeBridge.reset();
		m_simulateBatteryBridge.reset();
		m_simulateWifiBridge.reset();
		m_simulateBluetoothBridge.reset();
		m_simulateHotspotBridge.reset();
		m_simulateNotificationsBridge.reset();
		m_demoIntervalBridge.reset();
	}

private:

	void showDemoPage() {
		if (m_demoContainer) return;

		if (m_container) {
			lv_obj_add_flag(m_container, LV_OBJ_FLAG_HIDDEN);
		}

		m_demoContainer = create_page_container(m_parent);
		lv_obj_t* backBtn = nullptr;
		create_header(m_demoContainer, "UI Demo Mode", &backBtn);

		lv_obj_add_event_cb(
			backBtn,
			[](lv_event_t* e) {
				auto* instance = (DeveloperSettings*)lv_event_get_user_data(e);
				instance->hideDemoPage();
			},
			LV_EVENT_CLICKED, this);

		m_demoList = create_settings_list(m_demoContainer);

		// ── Master Switch ──
		lv_list_add_text(m_demoList, "Status Bar Automation");

		lv_obj_t* demoModeBtn = add_list_btn(m_demoList, LV_SYMBOL_PLAY, "Enable UI Demo Cycle");
		lv_obj_set_flex_grow(lv_obj_get_child(demoModeBtn, 1), 1);
		lv_obj_t* demoModeSw = lv_switch_create(demoModeBtn);
		lv_obj_add_flag(demoModeSw, LV_OBJ_FLAG_EVENT_BUBBLE);
		lv_obj_bind_checked(demoModeSw, m_demoModeBridge->getSubject());

		// ── Components Section ──
		lv_list_add_text(m_demoList, "Participating Components");

		// Battery
		lv_obj_t* battBtn = add_list_btn(m_demoList, LV_SYMBOL_CHARGE, "Simulate Battery");
		lv_obj_set_flex_grow(lv_obj_get_child(battBtn, 1), 1);
		lv_obj_t* battSw = lv_switch_create(battBtn);
		lv_obj_add_flag(battSw, LV_OBJ_FLAG_EVENT_BUBBLE);
		lv_obj_bind_checked(battSw, m_simulateBatteryBridge->getSubject());

		// Wi-Fi
		lv_obj_t* wifiBtn = add_list_btn(m_demoList, LV_SYMBOL_WIFI, "Simulate Wi-Fi");
		lv_obj_set_flex_grow(lv_obj_get_child(wifiBtn, 1), 1);
		lv_obj_t* wifiSw = lv_switch_create(wifiBtn);
		lv_obj_add_flag(wifiSw, LV_OBJ_FLAG_EVENT_BUBBLE);
		lv_obj_bind_checked(wifiSw, m_simulateWifiBridge->getSubject());

		// Bluetooth
		lv_obj_t* btBtn = add_list_btn(m_demoList, LV_SYMBOL_BLUETOOTH, "Simulate Bluetooth");
		lv_obj_set_flex_grow(lv_obj_get_child(btBtn, 1), 1);
		lv_obj_t* btSw = lv_switch_create(btBtn);
		lv_obj_add_flag(btSw, LV_OBJ_FLAG_EVENT_BUBBLE);
		lv_obj_bind_checked(btSw, m_simulateBluetoothBridge->getSubject());

		// Hotspot
		lv_obj_t* hotspotBtn = add_list_btn(m_demoList, LV_SYMBOL_WIFI, "Simulate Hotspot");
		lv_obj_set_flex_grow(lv_obj_get_child(hotspotBtn, 1), 1);
		lv_obj_t* hotspotSw = lv_switch_create(hotspotBtn);
		lv_obj_add_flag(hotspotSw, LV_OBJ_FLAG_EVENT_BUBBLE);
		lv_obj_bind_checked(hotspotSw, m_simulateHotspotBridge->getSubject());

		// Notifications
		lv_obj_t* notifBtn = add_list_btn(m_demoList, LV_SYMBOL_BELL, "Simulate Notifications");
		lv_obj_set_flex_grow(lv_obj_get_child(notifBtn, 1), 1);
		lv_obj_t* notifSw = lv_switch_create(notifBtn);
		lv_obj_add_flag(notifSw, LV_OBJ_FLAG_EVENT_BUBBLE);
		lv_obj_bind_checked(notifSw, m_simulateNotificationsBridge->getSubject());

		// ── Timing Section ──
		lv_list_add_text(m_demoList, "Simulation Timing");

		add_slider_item(m_demoList, "Interval (ms)", m_demoIntervalBridge->getSubject(), 500, 5000);
	}

	void hideDemoPage() {
		if (m_demoContainer) {
			lv_obj_delete(m_demoContainer);
			m_demoContainer = nullptr;
			m_demoList = nullptr;
		}
		if (m_container) {
			lv_obj_remove_flag(m_container, LV_OBJ_FLAG_HIDDEN);
		}
	}

	lv_obj_t* m_demoContainer = nullptr;
	lv_obj_t* m_demoList = nullptr;

	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_verboseLoggingBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_diagnosticOverlayBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_demoModeBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_simulateBatteryBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_simulateWifiBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_simulateBluetoothBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_simulateHotspotBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_simulateNotificationsBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_demoIntervalBridge;
};

} // namespace System::Apps::Settings
