#include "AdaptivePage.hpp"

#include <ctime>
#include <flx/system/managers/WallpaperManager.hpp>
#include <flx/ui/common/SettingsCommon.hpp>

using namespace flx::ui::common;

namespace System::Apps::WallpaperEngine {

AdaptivePage::AdaptivePage(lv_obj_t* parent, std::function<void()> onBack)
	: m_onBack(std::move(onBack)) {

	auto& wm = flx::system::WallpaperManager::getInstance();
	m_wpEnabledBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(
		wm.getWallpaperEnabledObservable());

	m_container = create_page_container(parent);

	// Header
	lv_obj_t* backBtn = nullptr;
	create_header(m_container, "Adaptive Wallpaper", &backBtn);
	add_back_button_event_cb(backBtn, &m_onBack);

	lv_obj_t* list = create_settings_list(m_container);

	// Info text
	lv_list_add_text(list, "Context-Aware Modes");

	// Time of Day mode button
	lv_obj_t* todBtn = add_list_btn(list, LV_SYMBOL_LEFT, "Time of Day");
	lv_obj_set_flex_grow(lv_obj_get_child(todBtn, 1), 1);
	lv_obj_t* todDesc = lv_label_create(todBtn);
	lv_label_set_text(todDesc, "Changes wallpaper by hour");
	lv_obj_set_style_text_opa(todDesc, LV_OPA_60, 0);

	lv_obj_add_event_cb(
		todBtn,
		[](lv_event_t* /*e*/) {
			// Determine time-of-day segment and pick a matching dynamic algo
			// This is a simple heuristic; a full AdaptiveProvider would do this.
			struct tm timeinfo {};
			time_t now = time(nullptr);
			localtime_r(&now, &timeinfo);
			int const hour = timeinfo.tm_hour;
			const char* source = "algo://gradient"; // default: calm
			if (hour >= 6 && hour < 12) {
				source = "algo://gradient"; // morning: soft gradient
			} else if (hour >= 12 && hour < 18) {
				source = "algo://plasma"; // afternoon: vibrant
			} else if (hour >= 18 && hour < 22) {
				source = "algo://perlin"; // evening: calm noise
			} else {
				source = "algo://perlin"; // night: dark calm
			}
			flx::system::WallpaperManager::getInstance().setWallpaper(source, "dynamic");
		},
		LV_EVENT_CLICKED, nullptr);

	// Battery-aware mode button
	lv_obj_t* batBtn = add_list_btn(list, LV_SYMBOL_BATTERY_2, "Battery Aware");
	lv_obj_set_flex_grow(lv_obj_get_child(batBtn, 1), 1);
	lv_obj_t* batDesc = lv_label_create(batBtn);
	lv_label_set_text(batDesc, "Static on low battery");
	lv_obj_set_style_text_opa(batDesc, LV_OPA_60, 0);

	lv_obj_add_event_cb(
		batBtn,
		[](lv_event_t* /*e*/) {
			// Simplified: switch to static (energy-saving) wallpaper
			flx::system::WallpaperManager::getInstance().setWallpaper("", "static");
		},
		LV_EVENT_CLICKED, nullptr);

	// Wallpaper enabled toggle
	lv_list_add_text(list, "General");
	lv_obj_t* enableBtn = add_list_btn(list, LV_SYMBOL_POWER, "Wallpaper Enabled");
	lv_obj_set_flex_grow(lv_obj_get_child(enableBtn, 1), 1);
	lv_obj_t* enableSw = lv_switch_create(enableBtn);
	lv_obj_bind_checked(enableSw, m_wpEnabledBridge->getSubject());

	lv_obj_add_flag(m_container, LV_OBJ_FLAG_HIDDEN);
}

void AdaptivePage::show() {
	if (m_container != nullptr) {
		lv_obj_remove_flag(m_container, LV_OBJ_FLAG_HIDDEN);
	}
}

void AdaptivePage::hide() {
	if (m_container != nullptr) {
		lv_obj_add_flag(m_container, LV_OBJ_FLAG_HIDDEN);
	}
}

} // namespace System::Apps::WallpaperEngine
