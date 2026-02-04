#include "HotspotSettings.hpp"
#include "core/apps/settings/SettingsCommon.hpp"
#include "core/connectivity/ConnectivityManager.hpp"
#include "core/connectivity/hotspot/HotspotManager.hpp"
#include "core/lv_obj.h"
#include "core/lv_obj_event.h"
#include "core/lv_obj_pos.h"
#include "core/lv_obj_style.h"
#include "core/lv_obj_style_gen.h"
#include "core/lv_obj_tree.h"
#include "core/lv_observer.h"
#include "core/ui/theming/layout_constants/LayoutConstants.hpp"
#include "core/ui/theming/ui_constants/UiConstants.hpp"
#include "display/lv_display.h"
#include "esp_err.h"
#include "esp_wifi_types_generic.h"
#include "font/lv_symbol_def.h"
#include "layouts/flex/lv_flex.h"
#include "misc/lv_anim.h"
#include "misc/lv_area.h"
#include "misc/lv_event.h"
#include "misc/lv_text.h"
#include "misc/lv_timer.h"
#include "misc/lv_types.h"
#include "widgets/button/lv_button.h"
#include "widgets/dropdown/lv_dropdown.h"
#include "widgets/image/lv_image.h"
#include "widgets/label/lv_label.h"
#include "widgets/slider/lv_slider.h"
#include "widgets/switch/lv_switch.h"
#include "widgets/textarea/lv_textarea.h"
#include <cstdint>

namespace System::Apps::Settings {

HotspotSettings::HotspotSettings(lv_obj_t* parent, std::function<void()> onBack)
	: m_parent(parent), m_onBack(onBack) {}

void HotspotSettings::show() {
	if (m_container == nullptr) {
		m_container = create_page_container(m_parent);

		createMainPage();
		createConfigPage();

		showMainPage();
	} else {
		lv_obj_remove_flag(m_container, LV_OBJ_FLAG_HIDDEN);
		showMainPage();
	}
}

void HotspotSettings::hide() {
	if (m_container) {
		lv_obj_add_flag(m_container, LV_OBJ_FLAG_HIDDEN);
	}
}

void HotspotSettings::destroy() {
	if (m_refreshTimer) {
		lv_timer_delete(m_refreshTimer);
		m_refreshTimer = nullptr;
	}
	m_container = nullptr;
	m_mainPage = nullptr;
	m_configPage = nullptr;
	m_hotspotSwitch = nullptr;
	m_ssidTa = nullptr;
	m_passwordTa = nullptr;
	m_channelDropdown = nullptr;
	m_maxConnSlider = nullptr;
	m_hiddenSwitch = nullptr;
	m_natSwitch = nullptr;
	m_securityDropdown = nullptr;
	m_txPowerSlider = nullptr;
	m_autoShutdownSwitch = nullptr;
	m_clientsCont = nullptr;
	m_configTitle = nullptr;
}

void HotspotSettings::createMainPage() {
	m_mainPage = create_page_container(m_container);

	lv_obj_t* backBtn = nullptr;
	lv_obj_t* header = create_header(m_mainPage, "Hotspot", &backBtn);
	add_back_button_event_cb(backBtn, &m_onBack);

	lv_obj_t* title = lv_obj_get_child(header, 1);
	lv_obj_set_flex_grow(title, 1);

	m_hotspotSwitch = lv_switch_create(header);
	lv_obj_bind_checked(
		m_hotspotSwitch,
		&ConnectivityManager::getInstance().getHotspotEnabledSubject()
	);

	lv_obj_add_event_cb(
		m_hotspotSwitch,
		[](lv_event_t* e) {
			auto* sw = lv_event_get_target_obj(e);
			auto* instance = (HotspotSettings*)lv_event_get_user_data(e);
			if (lv_obj_has_state(sw, LV_STATE_CHECKED)) {
				instance->applyHotspotSettings();
			} else {
				ConnectivityManager::getInstance().stopHotspot();
			}
		},
		LV_EVENT_VALUE_CHANGED, this
	);

	lv_obj_t* content = lv_obj_create(m_mainPage);
	lv_obj_set_size(content, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_grow(content, 1);
	lv_obj_set_style_border_width(content, 0, 0);
	lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
	lv_obj_set_style_pad_ver(content, lv_dpx(LayoutConstants::PAD_CONTAINER), 0);

	lv_obj_t* confBtn = lv_button_create(content);
	lv_obj_set_width(confBtn, lv_pct(100));
	lv_obj_set_style_margin_top(confBtn, 0, 0);
	lv_obj_set_flex_flow(confBtn, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(confBtn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_t* confImg = lv_image_create(confBtn);
	lv_image_set_src(confImg, LV_SYMBOL_SETTINGS);

	lv_obj_t* confLabel = lv_label_create(confBtn);
	lv_label_set_text(confLabel, " Configure Hotspot");
	lv_obj_add_event_cb(
		confBtn,
		[](lv_event_t* e) {
			auto* instance = (HotspotSettings*)lv_event_get_user_data(e);
			instance->showConfigPage();
		},
		LV_EVENT_CLICKED, this
	);

	lv_obj_t* usageHeader = lv_label_create(content);
	lv_label_set_text(usageHeader, "Data Usage:");
	lv_obj_set_style_margin_top(usageHeader, lv_dpx(LayoutConstants::MARGIN_SECTION), 0);

	lv_obj_t* usageCont = lv_obj_create(content);
	lv_obj_set_size(usageCont, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_style_pad_all(usageCont, lv_dpx(UiConstants::PAD_MEDIUM), 0);
	lv_obj_set_flex_flow(usageCont, LV_FLEX_FLOW_COLUMN);

	lv_obj_t* uptimeLabel = lv_label_create(usageCont);
	lv_label_set_text(uptimeLabel, "Uptime: 0s");
	lv_subject_add_observer_obj(
		&ConnectivityManager::getInstance().getHotspotUptimeLvglSubject(),
		[](lv_observer_t* o, lv_subject_t* s) {
			lv_obj_t* label = lv_observer_get_target_obj(o);
			int32_t const sec = lv_subject_get_int(s);
			int const h = sec / 3600;
			int const m = (sec % 3600) / 60;
			int const r = sec % 60;
			if (h > 0) {
				lv_label_set_text_fmt(label, "Uptime: %dh %dm %ds", h, m, r);
			} else if (m > 0) {
				lv_label_set_text_fmt(label, "Uptime: %dm %ds", m, r);
			} else {
				lv_label_set_text_fmt(label, "Uptime: %ds", r);
			}
		},
		uptimeLabel, nullptr
	);

	lv_obj_t* downSpeedLabel = lv_label_create(usageCont);
	lv_label_set_text(downSpeedLabel, "Download: 0 KB/s");
	lv_subject_add_observer_obj(
		&ConnectivityManager::getInstance().getHotspotDownloadSpeedLvglSubject(),
		[](lv_observer_t* o, lv_subject_t* s) {
			lv_obj_t* label = lv_observer_get_target_obj(o);
			int32_t const kb = lv_subject_get_int(s);
			if (kb > 1024) {
				lv_label_set_text_fmt(label, "Download: %.2f MB/s", (float)kb / 1024.0F);
			} else {
				lv_label_set_text_fmt(label, "Download: %d KB/s", (int)kb);
			}
		},
		downSpeedLabel, nullptr
	);

	lv_obj_t* upSpeedLabel = lv_label_create(usageCont);
	lv_label_set_text(upSpeedLabel, "Upload: 0 KB/s");
	lv_subject_add_observer_obj(
		&ConnectivityManager::getInstance().getHotspotUploadSpeedLvglSubject(),
		[](lv_observer_t* o, lv_subject_t* s) {
			lv_obj_t* label = lv_observer_get_target_obj(o);
			int32_t const kb = lv_subject_get_int(s);
			if (kb > 1024) {
				lv_label_set_text_fmt(label, "Upload: %.2f MB/s", (float)kb / 1024.0F);
			} else {
				lv_label_set_text_fmt(label, "Upload: %d KB/s", (int)kb);
			}
		},
		upSpeedLabel, nullptr
	);

	lv_obj_t* sentLabel = lv_label_create(usageCont);
	lv_label_set_text(sentLabel, "Sent: 0 KB");
	lv_subject_add_observer_obj(
		&ConnectivityManager::getInstance().getHotspotUsageSentLvglSubject(),
		[](lv_observer_t* o, lv_subject_t* s) {
			lv_obj_t* label = lv_observer_get_target_obj(o);
			int32_t const kb = lv_subject_get_int(s);
			if (kb > 1024) {
				lv_label_set_text_fmt(label, "Sent: %.2f MB", (float)kb / 1024.0F);
			} else {
				lv_label_set_text_fmt(label, "Sent: %d KB", (int)kb);
			}
		},
		sentLabel, nullptr
	);

	lv_obj_t* recvLabel = lv_label_create(usageCont);
	lv_label_set_text(recvLabel, "Received: 0 KB");
	lv_subject_add_observer_obj(
		&ConnectivityManager::getInstance().getHotspotUsageReceivedLvglSubject(),
		[](lv_observer_t* o, lv_subject_t* s) {
			lv_obj_t* label = lv_observer_get_target_obj(o);
			int32_t const kb = lv_subject_get_int(s);
			if (kb > 1024) {
				lv_label_set_text_fmt(label, "Received: %.2f MB", (float)kb / 1024.0F);
			} else {
				lv_label_set_text_fmt(label, "Received: %d KB", (int)kb);
			}
		},
		recvLabel, nullptr
	);

	lv_obj_t* resetUsageBtn = lv_button_create(usageCont);
	lv_obj_set_size(resetUsageBtn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_t* resetLabel = lv_label_create(resetUsageBtn);
	lv_label_set_text(resetLabel, "Reset Usage");
	lv_obj_add_event_cb(
		resetUsageBtn,
		[](lv_event_t* /*e*/) {
			HotspotManager::getInstance().resetUsage();
		},
		LV_EVENT_CLICKED, nullptr
	);

	lv_obj_t* clientsHeader = lv_label_create(content);
	lv_label_set_text(clientsHeader, "Connected Clients (0):");
	lv_obj_set_style_margin_top(clientsHeader, lv_dpx(LayoutConstants::MARGIN_SECTION), 0);

	lv_subject_add_observer_obj(
		&ConnectivityManager::getInstance().getHotspotClientsSubject(),
		[](lv_observer_t* o, lv_subject_t* s) {
			lv_obj_t* label = lv_observer_get_target_obj(o);
			lv_label_set_text_fmt(
				label, "Connected Clients (%d):", (int)lv_subject_get_int(s)
			);
		},
		clientsHeader, nullptr
	);

	m_clientsCont = lv_obj_create(content);
	lv_obj_set_size(m_clientsCont, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(m_clientsCont, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_all(m_clientsCont, lv_dpx(UiConstants::PAD_MEDIUM), 0);

	if (m_refreshTimer == nullptr) {
		m_refreshTimer = lv_timer_create(
			[](lv_timer_t* t) {
				auto* instance = (HotspotSettings*)lv_timer_get_user_data(t);
				if (!instance->m_clientsCont ||
					lv_obj_has_flag(instance->m_mainPage, LV_OBJ_FLAG_HIDDEN)) {
					return;
				}

				lv_obj_clean(instance->m_clientsCont);
				auto clients =
					ConnectivityManager::getInstance().getHotspotClientsList();
				if (clients.empty()) {
					lv_label_set_text(lv_label_create(instance->m_clientsCont), "No clients connected");
				} else {
					for (const auto& client: clients) {
						lv_obj_t* item = lv_obj_create(instance->m_clientsCont);
						lv_obj_set_size(item, lv_pct(100), LV_SIZE_CONTENT);
						lv_obj_set_style_pad_all(item, lv_dpx(UiConstants::PAD_MEDIUM), 0);
						lv_obj_set_flex_flow(item, LV_FLEX_FLOW_COLUMN);

						lv_obj_t* name_label = lv_label_create(item);
						if (!client.hostname.empty()) {
							lv_label_set_text_fmt(name_label, LV_SYMBOL_WIFI " %s", client.hostname.c_str());
						} else {
							lv_label_set_text_fmt(name_label, LV_SYMBOL_WIFI " Device %d", client.aid);
						}

						lv_obj_t* info_label = lv_label_create(item);
						const char* rssi_str = "Excellent";
						if (client.rssi < -80)
							rssi_str = "Poor";
						else if (client.rssi < -70)
							rssi_str = "Fair";
						else if (client.rssi < -60)
							rssi_str = "Good";

						uint32_t d = client.connected_duration;
						if (d > 3600) {
							lv_label_set_text_fmt(
								info_label, "IP: %s | %s | %dh %dm",
								client.ip.empty() ? "Allocating..." : client.ip.c_str(),
								rssi_str, (int)(d / 3600), (int)((d % 3600) / 60)
							);
						} else {
							lv_label_set_text_fmt(info_label, "IP: %s | %s | %dm %ds", client.ip.empty() ? "Allocating..." : client.ip.c_str(), rssi_str, (int)(d / 60), (int)(d % 60));
						}
					}
				}
			},
			2000, this
		);
	}
}

void HotspotSettings::createConfigPage() {
	m_configPage = create_page_container(m_container);
	lv_obj_add_flag(m_configPage, LV_OBJ_FLAG_HIDDEN);

	lv_obj_t* backBtn = nullptr;
	lv_obj_t* header =
		create_header(m_configPage, "Configure Hotspot", &backBtn);
	lv_obj_set_size(backBtn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_add_event_cb(
		backBtn,
		[](lv_event_t* e) {
			auto* instance = (HotspotSettings*)lv_event_get_user_data(e);
			instance->showMainPage();
		},
		LV_EVENT_CLICKED, this
	);

	m_configTitle = lv_obj_get_child(header, 1);
	lv_obj_set_flex_grow(m_configTitle, 1);

	lv_obj_t* saveBtn = lv_button_create(header);
	lv_obj_set_size(saveBtn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_t* saveLabel = lv_image_create(saveBtn);
	lv_image_set_src(saveLabel, LV_SYMBOL_OK);
	lv_obj_add_event_cb(
		saveBtn,
		[](lv_event_t* e) {
			auto* instance = (HotspotSettings*)lv_event_get_user_data(e);
			instance->saveAndApply();
		},
		LV_EVENT_CLICKED, this
	);

	lv_obj_t* content = lv_obj_create(m_configPage);
	lv_obj_set_size(content, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_grow(content, 1);
	lv_obj_set_style_border_width(content, 0, 0);
	lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_pad_all(content, lv_dpx(LayoutConstants::PAD_CONTAINER), 0);

	lv_obj_t* ssidLabel = lv_label_create(content);
	lv_label_set_text(ssidLabel, "Network Name (SSID):");
	m_ssidTa = lv_textarea_create(content);
	lv_textarea_set_one_line(m_ssidTa, true);

	const char* saved_ssid = (const char*)lv_subject_get_pointer(&ConnectivityManager::getInstance().getHotspotSsidSubject());
	lv_textarea_set_text(m_ssidTa, saved_ssid ? saved_ssid : "ESP32-Hotspot");
	lv_obj_set_width(m_ssidTa, lv_pct(100));

	lv_obj_t* passLabel = lv_label_create(content);
	lv_label_set_text(passLabel, "Password (min 8 chars):");
	lv_obj_set_style_margin_top(passLabel, lv_dpx(UiConstants::PAD_LARGE), 0);
	m_passwordTa = lv_textarea_create(content);
	lv_textarea_set_one_line(m_passwordTa, true);
	lv_textarea_set_password_mode(m_passwordTa, true);

	const char* saved_pass = (const char*)lv_subject_get_pointer(&ConnectivityManager::getInstance().getHotspotPasswordSubject());
	lv_textarea_set_text(m_passwordTa, saved_pass ? saved_pass : "12345678");
	lv_obj_set_width(m_passwordTa, lv_pct(100));

	lv_obj_t* advLabel = lv_label_create(content);
	lv_label_set_text(advLabel, "Advanced Settings:");
	lv_obj_set_style_margin_top(advLabel, lv_dpx(LayoutConstants::MARGIN_SECTION), 0);

	// Channel
	lv_obj_t* channelCont = lv_obj_create(content);
	lv_obj_set_size(channelCont, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_style_pad_all(channelCont, 0, 0);
	lv_obj_set_style_border_width(channelCont, 0, 0);
	lv_obj_set_flex_flow(channelCont, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(channelCont, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_t* channelLabel = lv_label_create(channelCont);
	lv_label_set_text(channelLabel, "WiFi Channel:");
	m_channelDropdown = lv_dropdown_create(channelCont);
	lv_dropdown_set_options(m_channelDropdown, "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13");
	int const saved_chan = lv_subject_get_int(&ConnectivityManager::getInstance().getHotspotChannelSubject());
	if (saved_chan >= 1 && saved_chan <= 13) lv_dropdown_set_selected(m_channelDropdown, saved_chan - 1);
	lv_obj_set_width(m_channelDropdown, lv_dpx(LayoutConstants::SIZE_DROPDOWN_WIDTH_SMALL));
	lv_dropdown_set_dir(m_channelDropdown, LV_DIR_LEFT);

	// Max Connections
	lv_obj_t* maxConnCont = lv_obj_create(content);
	lv_obj_set_size(maxConnCont, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_style_pad_all(maxConnCont, 0, 0);
	lv_obj_set_style_border_width(maxConnCont, 0, 0);
	lv_obj_set_flex_flow(maxConnCont, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(maxConnCont, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_t* maxConnLabel = lv_label_create(maxConnCont);
	lv_label_set_text(maxConnLabel, "Max Connections:");
	lv_obj_t* sliderCont = lv_obj_create(maxConnCont);
	lv_obj_set_size(sliderCont, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_style_pad_all(sliderCont, 0, 0);
	lv_obj_set_style_border_width(sliderCont, 0, 0);
	lv_obj_set_flex_flow(sliderCont, LV_FLEX_FLOW_COLUMN);
	m_maxConnSlider = lv_slider_create(sliderCont);
	lv_slider_set_range(m_maxConnSlider, 1, 10);
	int const saved_max = lv_subject_get_int(&ConnectivityManager::getInstance().getHotspotMaxConnSubject());
	lv_slider_set_value(m_maxConnSlider, saved_max > 0 ? saved_max : 4, LV_ANIM_OFF);
	lv_obj_set_width(m_maxConnSlider, lv_pct(100));
	lv_obj_t* maxConnValLabel = lv_label_create(sliderCont);
	lv_label_set_text_fmt(maxConnValLabel, "%d", saved_max > 0 ? saved_max : 4);
	lv_obj_set_style_text_align(maxConnValLabel, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_width(maxConnValLabel, lv_pct(100));
	lv_obj_add_event_cb(
		m_maxConnSlider,
		[](lv_event_t* e) {
			lv_obj_t* slider = lv_event_get_target_obj(e);
			auto* label = (lv_obj_t*)lv_event_get_user_data(e);
			lv_label_set_text_fmt(label, "%d", (int)lv_slider_get_value(slider));
		},
		LV_EVENT_VALUE_CHANGED, maxConnValLabel
	);

	// Hidden SSID
	lv_obj_t* hiddenCont = lv_obj_create(content);
	lv_obj_set_size(hiddenCont, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_style_pad_all(hiddenCont, 0, 0);
	lv_obj_set_style_border_width(hiddenCont, 0, 0);
	lv_obj_set_flex_flow(hiddenCont, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(hiddenCont, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_t* hiddenLabel = lv_label_create(hiddenCont);
	lv_label_set_text(hiddenLabel, "Hide SSID:");
	m_hiddenSwitch = lv_switch_create(hiddenCont);
	if (lv_subject_get_int(&ConnectivityManager::getInstance().getHotspotHiddenSubject())) {
		lv_obj_add_state(m_hiddenSwitch, LV_STATE_CHECKED);
	}

	// Internet Sharing (NAT)
	lv_obj_t* natCont = lv_obj_create(content);
	lv_obj_set_size(natCont, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_style_pad_all(natCont, 0, 0);
	lv_obj_set_style_border_width(natCont, 0, 0);
	lv_obj_set_flex_flow(natCont, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(natCont, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_t* natLabel = lv_label_create(natCont);
	lv_label_set_text(natLabel, "Internet Sharing (NAT):");
	m_natSwitch = lv_switch_create(natCont);
	if (ConnectivityManager::getInstance().isHotspotNatEnabled()) {
		lv_obj_add_state(m_natSwitch, LV_STATE_CHECKED);
	}

	// Security Mode
	lv_obj_t* secCont = lv_obj_create(content);
	lv_obj_set_size(secCont, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_style_pad_all(secCont, 0, 0);
	lv_obj_set_style_border_width(secCont, 0, 0);
	lv_obj_set_flex_flow(secCont, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(secCont, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_t* secLabel = lv_label_create(secCont);
	lv_label_set_text(secLabel, "Security:");
	m_securityDropdown = lv_dropdown_create(secCont);
	lv_dropdown_set_options(m_securityDropdown, "Open\nWPA2 PSK\nWPA3 PSK\nWPA2/WPA3");
	lv_dropdown_set_selected(m_securityDropdown, lv_subject_get_int(&ConnectivityManager::getInstance().getHotspotAuthSubject()));
	lv_obj_set_width(m_securityDropdown, lv_dpx(LayoutConstants::SIZE_DROPDOWN_WIDTH_LARGE));
	lv_dropdown_set_dir(m_securityDropdown, LV_DIR_LEFT);

	// TX Power
	lv_obj_t* txCont = lv_obj_create(content);
	lv_obj_set_size(txCont, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_style_pad_all(txCont, 0, 0);
	lv_obj_set_style_border_width(txCont, 0, 0);
	lv_obj_set_flex_flow(txCont, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(txCont, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_t* txLabel = lv_label_create(txCont);
	lv_label_set_text(txLabel, "TX Power:");
	lv_obj_t* txSliderCont = lv_obj_create(txCont);
	lv_obj_set_size(txSliderCont, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_style_pad_all(txSliderCont, 0, 0);
	lv_obj_set_style_border_width(txSliderCont, 0, 0);
	lv_obj_set_flex_flow(txSliderCont, LV_FLEX_FLOW_COLUMN);
	m_txPowerSlider = lv_slider_create(txSliderCont);
	lv_slider_set_range(m_txPowerSlider, 8, 80);
	lv_slider_set_value(m_txPowerSlider, 80, LV_ANIM_OFF);
	lv_obj_set_width(m_txPowerSlider, lv_pct(100));
	lv_obj_t* txValLabel = lv_label_create(txSliderCont);
	lv_label_set_text(txValLabel, "80");
	lv_obj_set_style_text_align(txValLabel, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_width(txValLabel, lv_pct(100));
	lv_obj_add_event_cb(
		m_txPowerSlider,
		[](lv_event_t* e) {
			lv_obj_t* slider = lv_event_get_target_obj(e);
			auto* label = (lv_obj_t*)lv_event_get_user_data(e);
			lv_label_set_text_fmt(label, "%d", (int)lv_slider_get_value(slider));
		},
		LV_EVENT_VALUE_CHANGED, txValLabel
	);

	// Auto Shutdown
	lv_obj_t* autoShutCont = lv_obj_create(content);
	lv_obj_set_size(autoShutCont, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_style_pad_all(autoShutCont, 0, 0);
	lv_obj_set_style_border_width(autoShutCont, 0, 0);
	lv_obj_set_flex_flow(autoShutCont, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(autoShutCont, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_t* autoShutLabel = lv_label_create(autoShutCont);
	lv_label_set_text(autoShutLabel, "Auto-Shutdown (5 min):");
	m_autoShutdownSwitch = lv_switch_create(autoShutCont);
	if (HotspotManager::getInstance().getAutoShutdownTimeout() > 0) {
		lv_obj_add_state(m_autoShutdownSwitch, LV_STATE_CHECKED);
	}
}

void HotspotSettings::showMainPage() {
	lv_obj_add_flag(m_configPage, LV_OBJ_FLAG_HIDDEN);
	lv_obj_remove_flag(m_mainPage, LV_OBJ_FLAG_HIDDEN);
}

void HotspotSettings::showConfigPage() {
	lv_obj_add_flag(m_mainPage, LV_OBJ_FLAG_HIDDEN);
	lv_obj_remove_flag(m_configPage, LV_OBJ_FLAG_HIDDEN);
}

void HotspotSettings::saveAndApply() {
	applyHotspotSettings();
	showMainPage();
}

void HotspotSettings::applyHotspotSettings() {
	const char* ssid = lv_textarea_get_text(m_ssidTa);
	const char* pass = lv_textarea_get_text(m_passwordTa);
	int const channel = lv_dropdown_get_selected(m_channelDropdown) + 1;
	int const max_conn = (int)lv_slider_get_value(m_maxConnSlider);
	bool const hidden = lv_obj_has_state(m_hiddenSwitch, LV_STATE_CHECKED);
	bool const nat_enabled = lv_obj_has_state(m_natSwitch, LV_STATE_CHECKED);

	wifi_auth_mode_t auth = WIFI_AUTH_WPA2_PSK;
	int const auth_idx = lv_dropdown_get_selected(m_securityDropdown);
	if (auth_idx == 0) {
		auth = WIFI_AUTH_OPEN;
	} else if (auth_idx == 1) {
		auth = WIFI_AUTH_WPA2_PSK;
	} else if (auth_idx == 2) {
		auth = WIFI_AUTH_WPA3_PSK;
	} else if (auth_idx == 3) {
		auth = WIFI_AUTH_WPA2_WPA3_PSK;
	}

	auto const tx_power = (int8_t)lv_slider_get_value(m_txPowerSlider);

	if (strlen(ssid) == 0) {
		lv_obj_remove_state(m_hotspotSwitch, LV_STATE_CHECKED);
		return;
	}

	if (auth != WIFI_AUTH_OPEN && strlen(pass) < 8) {
		lv_obj_remove_state(m_hotspotSwitch, LV_STATE_CHECKED);
		return;
	}

	// Save to system settings (triggers auto-save)
	static char ssid_buf[33];
	static char pass_buf[65];
	strncpy(ssid_buf, ssid, sizeof(ssid_buf) - 1);
	strncpy(pass_buf, pass, sizeof(pass_buf) - 1);

	lv_subject_set_pointer(&ConnectivityManager::getInstance().getHotspotSsidSubject(), ssid_buf);
	lv_subject_set_pointer(&ConnectivityManager::getInstance().getHotspotPasswordSubject(), pass_buf);
	lv_subject_set_int(&ConnectivityManager::getInstance().getHotspotChannelSubject(), channel);
	lv_subject_set_int(&ConnectivityManager::getInstance().getHotspotMaxConnSubject(), max_conn);
	lv_subject_set_int(&ConnectivityManager::getInstance().getHotspotHiddenSubject(), hidden ? 1 : 0);
	lv_subject_set_int(&ConnectivityManager::getInstance().getHotspotAuthSubject(), auth_idx);

	ConnectivityManager::getInstance().setHotspotNatEnabled(nat_enabled);
	HotspotManager::getInstance().setAutoShutdownTimeout(
		lv_obj_has_state(m_autoShutdownSwitch, LV_STATE_CHECKED) ? 300 : 0
	);

	// Only start/restart if the switch is ON
	if (lv_obj_has_state(m_hotspotSwitch, LV_STATE_CHECKED)) {
		esp_err_t const err = ConnectivityManager::getInstance().startHotspot(
			ssid, pass, channel, max_conn, hidden, auth, tx_power
		);
		if (err != ESP_OK) {
			lv_obj_remove_state(m_hotspotSwitch, LV_STATE_CHECKED);
		}
	}
}

} // namespace System::Apps::Settings
