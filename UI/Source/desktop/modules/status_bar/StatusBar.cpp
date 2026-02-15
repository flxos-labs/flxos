#include "core/lv_obj.h"
#include "core/lv_obj_pos.h"
#include "core/lv_obj_scroll.h"
#include "core/lv_obj_style.h"
#include "core/lv_obj_style_gen.h"
#include "core/lv_obj_tree.h"
#include "core/lv_observer.h"
#include "display/lv_display.h"
#include "flx/connectivity/ConnectivityManager.hpp"
#include "font/lv_symbol_def.h"
#include "layouts/flex/lv_flex.h"
#include "misc/lv_area.h"
#include "misc/lv_timer.h"
#include "misc/lv_types.h"
#include "widgets/image/lv_image.h"
#include "widgets/label/lv_label.h"
#include <cstdint>
#include <ctime>
#include <flx/core/EventBus.hpp>
#include <flx/system/SystemManager.hpp>
#include <flx/system/managers/NotificationManager.hpp>
#include <flx/system/managers/PowerManager.hpp>
#include <flx/ui/desktop/modules/status_bar/StatusBar.hpp>
#include <flx/ui/theming/StyleUtils.hpp>
#include <flx/ui/theming/layout_constants/LayoutConstants.hpp>
#include <flx/ui/theming/theme_engine/ThemeEngine.hpp>
#include <flx/ui/theming/themes/Themes.hpp>
#include <flx/ui/theming/ui_constants/UiConstants.hpp>
#include <string>

namespace UI::Modules {

lv_obj_t* StatusBar::s_overlayLabel = nullptr;
lv_obj_t* StatusBar::s_statusBarInstance = nullptr;

StatusBar::StatusBar(lv_obj_t* parent) : m_parent(parent) {
	create();
}

StatusBar::~StatusBar() {
	if (m_timer) {
		lv_timer_delete(m_timer);
	}
	s_overlayLabel = nullptr;
	s_statusBarInstance = nullptr;
}

void StatusBar::create() {
	m_statusBar = lv_obj_create(m_parent);
	s_statusBarInstance = m_statusBar;
	lv_obj_remove_style_all(m_statusBar);
	lv_obj_set_size(m_statusBar, lv_pct(100), lv_pct(UiConstants::SIZE_STATUS_BAR_HEIGHT_PCT));
	lv_obj_set_style_pad_hor(m_statusBar, lv_dpx(UiConstants::PAD_SMALL), 0);
	lv_obj_set_scroll_dir(m_statusBar, LV_DIR_NONE);

	UI::StyleUtils::apply_glass(m_statusBar, lv_dpx(UiConstants::GLASS_BLUR_DEFAULT));

	lv_obj_set_flex_flow(m_statusBar, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(m_statusBar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	auto& cm = flx::connectivity::ConnectivityManager::getInstance();
	m_wifiConnectedBridge.reset(new flx::ui::LvglObserverBridge<int32_t>(cm.getWiFiConnectedObservable()));
	m_wifiEnabledBridge.reset(new flx::ui::LvglObserverBridge<int32_t>(cm.getWiFiEnabledObservable()));
	m_hotspotEnabledBridge.reset(new flx::ui::LvglObserverBridge<int32_t>(cm.getHotspotEnabledObservable()));
	m_hotspotClientsBridge.reset(new flx::ui::LvglObserverBridge<int32_t>(cm.getHotspotClientsObservable()));
	m_bluetoothEnabledBridge.reset(new flx::ui::LvglObserverBridge<int32_t>(cm.getBluetoothEnabledObservable()));

	lv_obj_t* left_group = lv_obj_create(m_statusBar);
	lv_obj_remove_style_all(left_group);
	lv_obj_set_size(left_group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_flex_grow(left_group, 1);
	lv_obj_set_flex_flow(left_group, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(left_group, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	if (flx::system::SystemManager::getInstance().isSafeMode()) {
		lv_obj_t* safe_img = lv_image_create(left_group);
		lv_image_set_src(safe_img, LV_SYMBOL_WARNING);

		lv_obj_t* safe_label = lv_label_create(left_group);
		lv_label_set_text(safe_label, " SAFE MODE");
	}

	lv_obj_t* wifi_cont = lv_obj_create(left_group);
	lv_obj_remove_style_all(wifi_cont);
	lv_obj_set_size(wifi_cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_t* wifi_icon = lv_image_create(wifi_cont);
	lv_image_set_src(wifi_icon, LV_SYMBOL_WIFI);
	lv_obj_t* wifi_slash = lv_label_create(wifi_cont);
	lv_label_set_text(wifi_slash, "/");
	lv_obj_center(wifi_slash);
	lv_obj_add_flag(wifi_slash, LV_OBJ_FLAG_HIDDEN);

	// Redefine callback to capture this and use bridges
	auto wifi_update_cb = [](lv_observer_t* observer, lv_subject_t* /*subject*/) {
		StatusBar* instance = (StatusBar*)lv_observer_get_user_data(observer);
		lv_obj_t* cont = lv_observer_get_target_obj(observer);
		lv_obj_t* icon = lv_obj_get_child(cont, 0);
		lv_obj_t* slash = lv_obj_get_child(cont, 1);

		bool const connected = lv_subject_get_int(instance->m_wifiConnectedBridge->getSubject()) != 0;
		bool const enabled = lv_subject_get_int(instance->m_wifiEnabledBridge->getSubject()) != 0;

		if (connected) {
			lv_obj_set_style_opa(icon, UiConstants::OPA_COVER, 0);
			lv_obj_add_flag(slash, LV_OBJ_FLAG_HIDDEN);
		} else if (enabled) {
			lv_obj_set_style_opa(icon, UiConstants::OPA_GLASS_BG, 0);
			lv_obj_add_flag(slash, LV_OBJ_FLAG_HIDDEN);
		} else {
			lv_obj_set_style_opa(icon, UiConstants::OPA_GLASS_BG, 0);
			lv_obj_remove_flag(slash, LV_OBJ_FLAG_HIDDEN);
			lv_obj_set_style_text_opa(slash, UiConstants::OPA_GLASS_BG, 0);
		}
	};

	lv_subject_add_observer_obj(m_wifiConnectedBridge->getSubject(), wifi_update_cb, wifi_cont, this);
	lv_subject_add_observer_obj(m_wifiEnabledBridge->getSubject(), wifi_update_cb, wifi_cont, this);

	lv_obj_t* hotspot_icon = lv_obj_create(left_group);
	lv_obj_remove_style_all(hotspot_icon);
	lv_obj_set_size(hotspot_icon, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(hotspot_icon, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(hotspot_icon, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_t* hotspot_icon1 = lv_image_create(hotspot_icon);
	lv_image_set_src(hotspot_icon1, LV_SYMBOL_WIFI);
	lv_obj_set_style_transform_rotation(hotspot_icon1, -LayoutConstants::ROTATION_90_DEG, 0);
	lv_obj_set_style_transform_pivot_x(hotspot_icon1, lv_pct(50), 0);
	lv_obj_set_style_transform_pivot_y(hotspot_icon1, lv_pct(50), 0);

	lv_obj_t* hotspot_icon2 = lv_image_create(hotspot_icon);
	lv_image_set_src(hotspot_icon2, LV_SYMBOL_WIFI);
	lv_obj_set_style_transform_rotation(hotspot_icon2, LayoutConstants::ROTATION_90_DEG, 0);
	lv_obj_set_style_transform_pivot_x(hotspot_icon2, lv_pct(50), 0);
	lv_obj_set_style_transform_pivot_y(hotspot_icon2, lv_pct(50), 0);
	lv_obj_set_style_margin_left(hotspot_icon2, -lv_dpx(UiConstants::SIZE_ICON_OVERLAP), 0);

	lv_obj_t* hotspot_slash = lv_label_create(hotspot_icon);
	lv_label_set_text(hotspot_slash, "/");
	lv_obj_add_flag(hotspot_slash, LV_OBJ_FLAG_FLOATING);
	lv_obj_center(hotspot_slash);
	lv_obj_add_flag(hotspot_slash, LV_OBJ_FLAG_HIDDEN);

	lv_obj_t* hotspot_label = lv_label_create(hotspot_icon);

	lv_subject_add_observer_obj(
		m_hotspotEnabledBridge->getSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			lv_obj_t* cont = lv_observer_get_target_obj(observer);
			lv_obj_t* icon1 = lv_obj_get_child(cont, 0);
			lv_obj_t* icon2 = lv_obj_get_child(cont, 1);
			lv_obj_t* slash = lv_obj_get_child(cont, 2);
			if (lv_subject_get_int(subject)) {
				lv_obj_set_style_opa(icon1, UiConstants::OPA_COVER, 0);
				lv_obj_set_style_opa(icon2, UiConstants::OPA_COVER, 0);
				lv_obj_add_flag(slash, LV_OBJ_FLAG_HIDDEN);
			} else {
				lv_obj_set_style_opa(icon1, UiConstants::OPA_GLASS_BG, 0);
				lv_obj_set_style_opa(icon2, UiConstants::OPA_GLASS_BG, 0);
				lv_obj_remove_flag(slash, LV_OBJ_FLAG_HIDDEN);
				lv_obj_set_style_text_opa(slash, UiConstants::OPA_GLASS_BG, 0);
			}
		},
		hotspot_icon, nullptr
	);

	lv_subject_add_observer_obj(
		m_hotspotClientsBridge->getSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			lv_obj_t* label = lv_observer_get_target_obj(observer);
			int32_t const clients = lv_subject_get_int(subject);
			if (clients > 0) {
				lv_label_set_text_fmt(label, "%d", (int)clients);
			} else {
				lv_label_set_text(label, "");
			}
		},
		hotspot_label, nullptr
	);

	lv_obj_t* bt_cont = lv_obj_create(left_group);
	lv_obj_remove_style_all(bt_cont);
	lv_obj_set_size(bt_cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_t* bt_icon = lv_image_create(bt_cont);
	lv_image_set_src(bt_icon, LV_SYMBOL_BLUETOOTH);
	lv_obj_t* bt_slash = lv_label_create(bt_cont);
	lv_label_set_text(bt_slash, "/");
	lv_obj_center(bt_slash);
	lv_obj_add_flag(bt_slash, LV_OBJ_FLAG_HIDDEN);
	lv_subject_add_observer_obj(
		m_bluetoothEnabledBridge->getSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			lv_obj_t* cont = lv_observer_get_target_obj(observer);
			lv_obj_t* icon = lv_obj_get_child(cont, 0);
			lv_obj_t* slash = lv_obj_get_child(cont, 1);
			if (lv_subject_get_int(subject)) {
				lv_obj_set_style_opa(icon, UiConstants::OPA_COVER, 0);
				lv_obj_add_flag(slash, LV_OBJ_FLAG_HIDDEN);
			} else {
				lv_obj_set_style_opa(icon, UiConstants::OPA_GLASS_BG, 0);
				lv_obj_remove_flag(slash, LV_OBJ_FLAG_HIDDEN);
				lv_obj_set_style_text_opa(slash, UiConstants::OPA_GLASS_BG, 0);
			}
		},
		bt_cont, nullptr
	);

	lv_obj_t* notif_btn = lv_obj_create(left_group);
	lv_obj_remove_style_all(notif_btn);
	lv_obj_set_size(notif_btn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(notif_btn, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(notif_btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_t* notif_icon = lv_image_create(notif_btn);
	lv_image_set_src(notif_icon, LV_SYMBOL_BELL);

	lv_label_create(notif_btn);

	m_unreadCountBridge.reset(new flx::ui::LvglObserverBridge<int32_t>(flx::system::NotificationManager::getInstance().getUnreadCountObservable()));
	lv_subject_add_observer_obj(
		m_unreadCountBridge->getSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			lv_obj_t* btn = lv_observer_get_target_obj(observer);
			lv_obj_t* badge = lv_obj_get_child(btn, 1);
			lv_obj_t* icon = lv_obj_get_child(btn, 0);

			int32_t const count = lv_subject_get_int(subject);
			if (count > 0) {
				lv_label_set_text_fmt(badge, "%d", (int)count);
				lv_obj_set_style_image_opa(icon, UiConstants::OPA_COVER, 0);
				lv_obj_remove_flag(btn, LV_OBJ_FLAG_HIDDEN);
			} else {
				lv_label_set_text(badge, "");
				lv_obj_set_style_image_opa(icon, UiConstants::OPA_40, 0);
				lv_obj_add_flag(btn, LV_OBJ_FLAG_HIDDEN);
			}
		},
		notif_btn, nullptr
	);

	// Battery Indicator
	if (flx::system::PowerManager::getInstance().getIsConfiguredObservable().get()) {
		lv_obj_t* batt_cont = lv_obj_create(m_statusBar);
		lv_obj_remove_style_all(batt_cont);
		lv_obj_set_size(batt_cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		lv_obj_set_flex_flow(batt_cont, LV_FLEX_FLOW_ROW);
		lv_obj_set_flex_align(batt_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		lv_image_create(batt_cont);
		lv_label_create(batt_cont);

		m_batteryLevelBridge.reset(new flx::ui::LvglObserverBridge<int32_t>(flx::system::PowerManager::getInstance().getBatteryLevelObservable()));
		lv_subject_add_observer_obj(
			m_batteryLevelBridge->getSubject(),
			[](lv_observer_t* observer, lv_subject_t* subject) {
				lv_obj_t* cont = lv_observer_get_target_obj(observer);
				lv_obj_t* icon = lv_obj_get_child(cont, 0);
				lv_obj_t* label = lv_obj_get_child(cont, 1);
				int32_t const level = lv_subject_get_int(subject);

				lv_label_set_text_fmt(label, "%d%%", (int)level);
				if (level < 20) {
					lv_image_set_src(icon, LV_SYMBOL_BATTERY_EMPTY);
					lv_obj_set_style_text_color(label, Themes::GetConfig(ThemeEngine::get_current_theme()).error, 0);
				} else if (level < 50) {
					lv_image_set_src(icon, LV_SYMBOL_BATTERY_1);
					lv_obj_set_style_text_color(label, Themes::GetConfig(ThemeEngine::get_current_theme()).secondary, 0);
				} else if (level < 80) {
					lv_image_set_src(icon, LV_SYMBOL_BATTERY_2);
					lv_obj_set_style_text_color(label, Themes::GetConfig(ThemeEngine::get_current_theme()).text_primary, 0);
				} else {
					lv_image_set_src(icon, LV_SYMBOL_BATTERY_FULL);
					lv_obj_set_style_text_color(label, Themes::GetConfig(ThemeEngine::get_current_theme()).text_primary, 0);
				}

				// Color based on level
				if (level <= 15) {
					ThemeConfig const cfg = Themes::GetConfig(ThemeEngine::get_current_theme());
					lv_obj_set_style_image_recolor(icon, cfg.error, 0);
					lv_obj_set_style_image_recolor_opa(icon, UiConstants::OPA_COVER, 0);
				} else {
					// Fallback to theme primary color via status bar inheritance
					ThemeConfig const cfg = Themes::GetConfig(ThemeEngine::get_current_theme());
					lv_obj_set_style_image_recolor(icon, cfg.text_primary, 0);
					lv_obj_set_style_image_recolor_opa(icon, UiConstants::OPA_COVER, 0);
				}
			},
			batt_cont, nullptr
		);
	}

	s_overlayLabel = lv_label_create(m_statusBar);
	lv_obj_set_style_text_color(s_overlayLabel, lv_color_hex(0xFFD700), 0);
	lv_obj_add_flag(s_overlayLabel, LV_OBJ_FLAG_HIDDEN);

	m_timeLabel = lv_label_create(m_statusBar);

	time_t now = 0;
	struct tm timeinfo = {};
	time(&now);
	localtime_r(&now, &timeinfo);
	lv_label_set_text_fmt(m_timeLabel, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);

	lv_timer_create(
		[](lv_timer_t* t) {
			auto* label = (lv_obj_t*)lv_timer_get_user_data(t);
			time_t now = 0;
			struct tm timeinfo = {};
			time(&now);
			localtime_r(&now, &timeinfo);
			lv_label_set_text_fmt(label, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
		},
		1000, m_timeLabel
	);

	// Event subscriptions for Overlay
	flx::core::EventBus::getInstance().subscribe("ui.overlay.show", [this](const std::string& /*event*/, const flx::core::Bundle& data) {
		std::string const text = data.getString("text");
		if (s_overlayLabel) {
			lv_label_set_text(s_overlayLabel, text.c_str());
			lv_obj_remove_flag(s_overlayLabel, LV_OBJ_FLAG_HIDDEN);
		}
	});

	flx::core::EventBus::getInstance().subscribe("ui.overlay.clear", [this](const std::string& /*event*/, const flx::core::Bundle& /*data*/) {
		if (s_overlayLabel) {
			lv_obj_add_flag(s_overlayLabel, LV_OBJ_FLAG_HIDDEN);
		}
	});
}

} // namespace UI::Modules
