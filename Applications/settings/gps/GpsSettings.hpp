#pragma once

#include <flx/hal/DeviceRegistry.hpp>
#include <flx/hal/IDevice.hpp>
#include <flx/hal/gps/IGpsDevice.hpp>
#include <lvgl.h>
#include <memory>
#include <string>

#include "settings/SettingsPageBase.hpp"

using namespace flx::ui::common;

namespace System::Apps::Settings {

class GpsSettings : public SettingsPageBase {
public:

	using SettingsPageBase::SettingsPageBase;

protected:

	void createUI() override {
		m_container = create_page_container(m_parent);

		lv_obj_t* backBtn = nullptr;
		create_header(m_container, "GPS / GNSS", &backBtn);
		add_back_button_event_cb(backBtn, &m_onBack);

		m_list = create_settings_list(m_container);

		buildUi();
	}

	void onShow() override {
		refresh();
		startRefreshTimer();
	}

	void onHide() override {
		stopRefreshTimer();
	}

	void onDestroy() override {
		stopRefreshTimer();
		m_stateLabel = nullptr;
		m_modelLabel = nullptr;
		m_latLabel = nullptr;
		m_lonLabel = nullptr;
		m_altLabel = nullptr;
		m_speedLabel = nullptr;
		m_hdopLabel = nullptr;
		m_satsLabel = nullptr;
		m_sleepSwitch = nullptr;
	}

private:

	void buildUi() {
		auto gps = getGpsDevice();

		if (!gps) {
			lv_list_add_text(m_list, "No GPS device registered");
			return;
		}

		// ── Device Info ──
		lv_list_add_text(m_list, "Device");

		lv_obj_t* modelBtn = add_list_btn(m_list, LV_SYMBOL_GPS, "Model");
		lv_obj_set_flex_grow(lv_obj_get_child(modelBtn, 1), 1);
		m_modelLabel = lv_label_create(modelBtn);
		lv_label_set_text(m_modelLabel, std::string(gps->getGpsModel()).c_str());

		lv_obj_t* stateBtn = add_list_btn(m_list, LV_SYMBOL_GPS, "State");
		lv_obj_set_flex_grow(lv_obj_get_child(stateBtn, 1), 1);
		m_stateLabel = lv_label_create(stateBtn);
		lv_label_set_text(m_stateLabel, "--");

		// ── Position ──
		lv_list_add_text(m_list, "Position");

		lv_obj_t* latBtn = add_list_btn(m_list, LV_SYMBOL_GPS, "Latitude");
		lv_obj_set_flex_grow(lv_obj_get_child(latBtn, 1), 1);
		m_latLabel = lv_label_create(latBtn);
		lv_label_set_text(m_latLabel, "--");

		lv_obj_t* lonBtn = add_list_btn(m_list, LV_SYMBOL_GPS, "Longitude");
		lv_obj_set_flex_grow(lv_obj_get_child(lonBtn, 1), 1);
		m_lonLabel = lv_label_create(lonBtn);
		lv_label_set_text(m_lonLabel, "--");

		lv_obj_t* altBtn = add_list_btn(m_list, LV_SYMBOL_UP, "Altitude (m)");
		lv_obj_set_flex_grow(lv_obj_get_child(altBtn, 1), 1);
		m_altLabel = lv_label_create(altBtn);
		lv_label_set_text(m_altLabel, "--");

		lv_obj_t* speedBtn = add_list_btn(m_list, LV_SYMBOL_PLAY, "Speed (kn)");
		lv_obj_set_flex_grow(lv_obj_get_child(speedBtn, 1), 1);
		m_speedLabel = lv_label_create(speedBtn);
		lv_label_set_text(m_speedLabel, "--");

		// ── Signal Quality ──
		lv_list_add_text(m_list, "Signal Quality");

		lv_obj_t* hdopBtn = add_list_btn(m_list, LV_SYMBOL_WIFI, "HDOP");
		lv_obj_set_flex_grow(lv_obj_get_child(hdopBtn, 1), 1);
		m_hdopLabel = lv_label_create(hdopBtn);
		lv_label_set_text(m_hdopLabel, "--");

		lv_obj_t* satsBtn = add_list_btn(m_list, LV_SYMBOL_WIFI, "Satellites");
		lv_obj_set_flex_grow(lv_obj_get_child(satsBtn, 1), 1);
		m_satsLabel = lv_label_create(satsBtn);
		lv_label_set_text(m_satsLabel, "--");

		// ── Controls ──
		lv_list_add_text(m_list, "GPS Controls");

		// Sleep toggle
		lv_obj_t* sleepBtn = add_list_btn(m_list, LV_SYMBOL_POWER, "RF Sleep (Save Power)");
		lv_obj_set_flex_grow(lv_obj_get_child(sleepBtn, 1), 1);
		m_sleepSwitch = lv_switch_create(sleepBtn);
		lv_obj_add_event_cb(
			m_sleepSwitch,
			[](lv_event_t* e) {
				auto* sw = lv_event_get_target_obj(e);
				bool const sleeping = lv_obj_has_state(sw, LV_STATE_CHECKED);
				auto gpsDevice = flx::hal::DeviceRegistry::getInstance()
									 .findFirst<flx::hal::gps::IGpsDevice>(flx::hal::IDevice::Type::Gps);
				if (gpsDevice) {
					gpsDevice->requestSleep(sleeping);
				}
			},
			LV_EVENT_VALUE_CHANGED, nullptr);

		// Cold Start button
		lv_obj_t* coldStartBtn = lv_list_add_button(m_list, LV_SYMBOL_REFRESH, "Request Cold Start");
		lv_obj_add_event_cb(
			coldStartBtn,
			[](lv_event_t* /*e*/) {
				auto gpsDevice = flx::hal::DeviceRegistry::getInstance()
									 .findFirst<flx::hal::gps::IGpsDevice>(flx::hal::IDevice::Type::Gps);
				if (gpsDevice) {
					gpsDevice->requestColdStart();
				}
			},
			LV_EVENT_CLICKED, nullptr);
	}

	void refresh() {
		auto gps = getGpsDevice();
		if (!gps) {
			return;
		}

		// Update state label
		if (m_stateLabel) {
			auto state = gps->getGpsState();
			const char* stateStr = "--";
			switch (state) {
				case flx::hal::gps::IGpsDevice::GpsState::Off:
					stateStr = "Off";
					break;
				case flx::hal::gps::IGpsDevice::GpsState::Searching:
					stateStr = "Searching...";
					break;
				case flx::hal::gps::IGpsDevice::GpsState::FixAcquired:
					stateStr = "Fix Acquired";
					break;
				case flx::hal::gps::IGpsDevice::GpsState::Error:
					stateStr = "Error";
					break;
			}
			lv_label_set_text(m_stateLabel, stateStr);
		}

		// Update position fields
		auto pos = gps->getLastPosition();
		if (pos.valid) {
			char buf[48];
			if (m_latLabel) {
				snprintf(buf, sizeof(buf), "%.6f", pos.latitude);
				lv_label_set_text(m_latLabel, buf);
			}
			if (m_lonLabel) {
				snprintf(buf, sizeof(buf), "%.6f", pos.longitude);
				lv_label_set_text(m_lonLabel, buf);
			}
			if (m_altLabel) {
				snprintf(buf, sizeof(buf), "%.1f m", pos.altitude);
				lv_label_set_text(m_altLabel, buf);
			}
			if (m_speedLabel) {
				snprintf(buf, sizeof(buf), "%.1f kn", pos.speedKnots);
				lv_label_set_text(m_speedLabel, buf);
			}
			if (m_hdopLabel) {
				snprintf(buf, sizeof(buf), "%.2f", pos.hdop);
				lv_label_set_text(m_hdopLabel, buf);
			}
			if (m_satsLabel) {
				snprintf(buf, sizeof(buf), "%d", (int)pos.satellitesUsed);
				lv_label_set_text(m_satsLabel, buf);
			}
		} else {
			const char* noFix = "No Fix";
			if (m_latLabel) lv_label_set_text(m_latLabel, noFix);
			if (m_lonLabel) lv_label_set_text(m_lonLabel, noFix);
			if (m_altLabel) lv_label_set_text(m_altLabel, noFix);
			if (m_speedLabel) lv_label_set_text(m_speedLabel, noFix);
			if (m_hdopLabel) lv_label_set_text(m_hdopLabel, noFix);
			if (m_satsLabel) lv_label_set_text(m_satsLabel, "0");
		}
	}

	void startRefreshTimer() {
		if (m_refreshTimer) {
			return;
		}
		m_refreshTimer = lv_timer_create(
			[](lv_timer_t* t) {
				auto* instance = (GpsSettings*)lv_timer_get_user_data(t);
				if (instance) {
					instance->refresh();
				}
			},
			1000, this);
	}

	void stopRefreshTimer() {
		if (m_refreshTimer) {
			lv_timer_delete(m_refreshTimer);
			m_refreshTimer = nullptr;
		}
	}

	static std::shared_ptr<flx::hal::gps::IGpsDevice> getGpsDevice() {
		return flx::hal::DeviceRegistry::getInstance()
			.findFirst<flx::hal::gps::IGpsDevice>(flx::hal::IDevice::Type::Gps);
	}

	lv_obj_t* m_stateLabel = nullptr;
	lv_obj_t* m_modelLabel = nullptr;
	lv_obj_t* m_latLabel = nullptr;
	lv_obj_t* m_lonLabel = nullptr;
	lv_obj_t* m_altLabel = nullptr;
	lv_obj_t* m_speedLabel = nullptr;
	lv_obj_t* m_hdopLabel = nullptr;
	lv_obj_t* m_satsLabel = nullptr;
	lv_obj_t* m_sleepSwitch = nullptr;
	lv_timer_t* m_refreshTimer = nullptr;
};

} // namespace System::Apps::Settings
