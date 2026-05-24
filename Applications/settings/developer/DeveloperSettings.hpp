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

using namespace flx::ui::common;
using namespace flx::system;

namespace System::Apps::Settings {

class DeveloperSettings : public SettingsPageBase {
public:

	using SettingsPageBase::SettingsPageBase;

	static inline flx::Observable<int32_t> verboseLogging {0};
	static inline flx::Observable<int32_t> diagnosticOverlay {0};

protected:

	void createUI() override {
		static bool registered = false;
		if (!registered) {
			SettingsManager::getInstance().registerSetting("developer.verbose_logging", verboseLogging);
			SettingsManager::getInstance().registerSetting("developer.diagnostic_overlay", diagnosticOverlay);
			registered = true;
		}

		m_verboseLoggingBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(verboseLogging);
		m_diagnosticOverlayBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(diagnosticOverlay);

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
		m_verboseLoggingBridge.reset();
		m_diagnosticOverlayBridge.reset();
	}

private:

	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_verboseLoggingBridge;
	std::unique_ptr<flx::ui::LvglObserverBridge<int32_t>> m_diagnosticOverlayBridge;
};

} // namespace System::Apps::Settings
