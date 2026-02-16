#pragma once

#include "settings/bluetooth/BluetoothSettings.hpp"
#include "settings/display/DisplaySettings.hpp"
#include "settings/hotspot/HotspotSettings.hpp"
#include "settings/wifi/WiFiSettings.hpp"
#include <flx/apps/App.hpp>
#include <flx/apps/AppManifest.hpp>
#include <flx/ui/common/SettingsCommon.hpp>
#include <memory>

using namespace flx::ui::common;

namespace System::Apps {

using flx::apps::AppManifest;
class SettingsApp : public flx::apps::App {
public:

	std::string getPackageName() const override { return "com.flxos.settings"; }
	std::string getAppName() const override { return "Settings"; }
	std::string getVersion() const override { return "1.1.0"; }
	const void* getIcon() const override { return LV_SYMBOL_SETTINGS; }

	static const AppManifest manifest;

	void createUI(void* parent) override {
		m_container = (lv_obj_t*)parent;
		m_mainList = nullptr;
		m_wifiSettings = std::make_unique<Settings::WiFiSettings>(
			m_container, [this]() { showMainSettings(); }
		);
		m_hotspotSettings = std::make_unique<Settings::HotspotSettings>(
			m_container, [this]() { showMainSettings(); }
		);
		m_bluetoothSettings = std::make_unique<Settings::BluetoothSettings>(
			m_container, [this]() { showMainSettings(); }
		);
		m_displaySettings = std::make_unique<Settings::DisplaySettings>(
			m_container, [this]() { showMainSettings(); }
		);
		showMainSettings();
	}

	void onStop() override {
		m_container = nullptr;
		m_mainList = nullptr;
		if (m_wifiSettings)
			m_wifiSettings->destroy();
		if (m_hotspotSettings)
			m_hotspotSettings->destroy();
		if (m_bluetoothSettings)
			m_bluetoothSettings->destroy();
		if (m_displaySettings)
			m_displaySettings->destroy();
		m_wifiSettings.reset();
		m_hotspotSettings.reset();
		m_bluetoothSettings.reset();
		m_displaySettings.reset();
	}

private:

	lv_obj_t* m_container = nullptr;
	lv_obj_t* m_mainList = nullptr;
	std::unique_ptr<Settings::WiFiSettings> m_wifiSettings;
	std::unique_ptr<Settings::HotspotSettings> m_hotspotSettings;
	std::unique_ptr<Settings::BluetoothSettings> m_bluetoothSettings;
	std::unique_ptr<Settings::DisplaySettings> m_displaySettings;

	void showMainSettings() {
		if (m_displaySettings)
			m_displaySettings->hide();
		if (m_wifiSettings)
			m_wifiSettings->hide();
		if (m_hotspotSettings)
			m_hotspotSettings->hide();
		if (m_bluetoothSettings)
			m_bluetoothSettings->hide();

		if (m_mainList == nullptr) {
			m_mainList = lv_list_create(m_container);
			lv_obj_set_size(m_mainList, lv_pct(100), lv_pct(100));
			lv_obj_set_style_border_width(m_mainList, 0, 0);

			lv_list_add_text(m_mainList, "Connectivity");
			lv_obj_t* wifiBtn =
				add_list_btn(m_mainList, LV_SYMBOL_WIFI, "Wi-Fi");
			lv_obj_add_event_cb(
				wifiBtn,
				[](lv_event_t* e) {
					auto* app = (SettingsApp*)lv_event_get_user_data(e);
					app->showWiFiSettings();
				},
				LV_EVENT_CLICKED, this
			);

			lv_obj_t* hotspotBtn =
				add_list_btn(m_mainList, LV_SYMBOL_WIFI,
							 "Hotspot"); // LV_SYMBOL_WIFI is used for
			// hotspot too if no specific one
			lv_obj_add_event_cb(
				hotspotBtn,
				[](lv_event_t* e) {
					auto* app = (SettingsApp*)lv_event_get_user_data(e);
					app->showHotspotSettings();
				},
				LV_EVENT_CLICKED, this
			);

			lv_obj_t* btBtn =
				add_list_btn(m_mainList, LV_SYMBOL_BLUETOOTH, "Bluetooth");
			lv_obj_add_event_cb(
				btBtn,
				[](lv_event_t* e) {
					auto* app = (SettingsApp*)lv_event_get_user_data(e);
					app->showBluetoothSettings();
				},
				LV_EVENT_CLICKED, this
			);

			lv_list_add_text(m_mainList, "System");
			lv_obj_t* displayBtn =
				add_list_btn(m_mainList, LV_SYMBOL_IMAGE, "Display");
			lv_obj_add_event_cb(
				displayBtn,
				[](lv_event_t* e) {
					auto* app = (SettingsApp*)lv_event_get_user_data(e);
					app->showDisplaySettings();
				},
				LV_EVENT_CLICKED, this
			);
		} else {
			lv_obj_remove_flag(m_mainList, LV_OBJ_FLAG_HIDDEN);
		}
	}

	void showWiFiSettings() {
		if (m_mainList) {
			lv_obj_add_flag(m_mainList, LV_OBJ_FLAG_HIDDEN);
		}

		if (m_wifiSettings) {
			m_wifiSettings->show();
		}
	}

	void showHotspotSettings() {
		if (m_mainList) {
			lv_obj_add_flag(m_mainList, LV_OBJ_FLAG_HIDDEN);
		}

		if (m_hotspotSettings) {
			m_hotspotSettings->show();
		}
	}

	void showBluetoothSettings() {
		if (m_mainList) {
			lv_obj_add_flag(m_mainList, LV_OBJ_FLAG_HIDDEN);
		}

		if (m_bluetoothSettings) {
			m_bluetoothSettings->show();
		}
	}

	void showDisplaySettings() {
		if (m_mainList) {
			lv_obj_add_flag(m_mainList, LV_OBJ_FLAG_HIDDEN);
		}

		if (m_displaySettings) {
			m_displaySettings->show();
		}
	}
};

} // namespace System::Apps
