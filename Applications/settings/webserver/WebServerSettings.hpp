#pragma once

#include <flx/connectivity/ConnectivityManager.hpp>
#include <flx/core/Observable.hpp>
#include <flx/system/managers/SettingsManager.hpp>
#include <flx/ui/LvglObserverBridge.hpp>
#include <lvgl.h>
#include <memory>

#include "settings/SettingsPageBase.hpp"

using namespace flx::ui::common;
using namespace flx::system;
using namespace flx::connectivity;

namespace System::Apps::Settings {

class WebServerSettings : public SettingsPageBase {
public:

	using SettingsPageBase::SettingsPageBase;

	static inline flx::Observable<int32_t> webserverEnabled {0};
	static inline flx::Observable<int32_t> webserverPort {80};

protected:

	void createUI() override {
		static bool registered = false;
		if (!registered) {
			SettingsManager::getInstance().registerSetting("webserver.enabled", webserverEnabled);
			SettingsManager::getInstance().registerSetting("webserver.port", webserverPort);
			registered = true;
		}

		m_enabledBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(webserverEnabled);
		m_portBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(webserverPort);
		m_ipBridge = std::make_unique<flx::ui::LvglStringObserverBridge>(ConnectivityManager::getInstance().getWiFiIpObservable());

		m_container = create_page_container(m_parent);

		lv_obj_t* backBtn = nullptr;
		create_header(m_container, "Web Server Settings", &backBtn);
		add_back_button_event_cb(backBtn, &m_onBack);

		m_list = create_settings_list(m_container);

		// ── Server Controls ──
		lv_list_add_text(m_list, "Configuration");

		lv_obj_t* enableBtn = add_list_btn(m_list, LV_SYMBOL_POWER, "Enable Web Server");
		lv_obj_set_flex_grow(lv_obj_get_child(enableBtn, 1), 1);
		lv_obj_t* enableSw = lv_switch_create(enableBtn);
		lv_obj_add_flag(enableSw, LV_OBJ_FLAG_EVENT_BUBBLE);
		lv_obj_bind_checked(enableSw, m_enabledBridge->getSubject());

		add_slider_item(m_list, "Port", m_portBridge->getSubject(), 80, 8080);

		// ── Connection Status ──
		lv_list_add_text(m_list, "Connection Status");

		lv_obj_t* ipBtn = add_list_btn(m_list, LV_SYMBOL_WIFI, "Local IP Address");
		lv_obj_set_flex_grow(lv_obj_get_child(ipBtn, 1), 1);

		m_ipLabel = lv_label_create(ipBtn);
		lv_label_set_text(m_ipLabel, ConnectivityManager::getInstance().getWiFiIpObservable().get().c_str());

		lv_subject_add_observer_obj(
			m_ipBridge->getSubject(),
			[](lv_observer_t* observer, lv_subject_t* subject) {
				auto* label = lv_observer_get_target_obj(observer);
				if (!label) {
					return;
				}
				char const* ipStr = static_cast<const char*>(lv_subject_get_pointer(subject));
				lv_label_set_text(label, ipStr ? ipStr : "0.0.0.0");
			},
			m_ipLabel, nullptr);
	}

	void onDestroy() override {
		m_enabledBridge.reset();
		m_portBridge.reset();
		m_ipBridge.reset();
		m_ipLabel = nullptr;
	}

private:

	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_enabledBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_portBridge;
	std::unique_ptr<flx::ui::LvglStringObserverBridge> m_ipBridge;
	lv_obj_t* m_ipLabel = nullptr;
};

} // namespace System::Apps::Settings
