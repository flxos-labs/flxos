#include "EffectsPage.hpp"

#include "../widgets/EffectSliderRow.hpp"
#include <flx/system/managers/WallpaperManager.hpp>
#include <flx/ui/common/SettingsCommon.hpp>

using namespace flx::ui::common;

namespace System::Apps::WallpaperEngine {

EffectsPage::EffectsPage(lv_obj_t* parent, std::function<void()> onBack)
	: m_onBack(std::move(onBack)) {

	auto& wm = flx::system::WallpaperManager::getInstance();

	m_wpEnabledBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(
		wm.getWallpaperEnabledObservable());
	m_animSpeedBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(
		wm.getAnimationSpeedObservable());
	m_qualityBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(
		wm.getQualityLevelObservable());

	m_container = create_page_container(parent);

	// Header
	lv_obj_t* backBtn = nullptr;
	create_header(m_container, "Wallpaper Effects", &backBtn);
	add_back_button_event_cb(backBtn, &m_onBack);

	// Scrollable list
	lv_obj_t* list = create_settings_list(m_container);

	// --- Enable wallpaper toggle ---
	lv_obj_t* enableBtn = add_list_btn(list, LV_SYMBOL_POWER, "Enable Wallpaper");
	lv_obj_set_flex_grow(lv_obj_get_child(enableBtn, 1), 1);
	lv_obj_t* enableSw = lv_switch_create(enableBtn);
	lv_obj_bind_checked(enableSw, m_wpEnabledBridge->getSubject());

	// --- Animation speed ---
	lv_obj_t* speedBtn = add_list_btn(list, LV_SYMBOL_PLAY, "Animation Speed");
	lv_obj_t* speedSlider = lv_slider_create(speedBtn);
	lv_obj_set_flex_grow(speedSlider, 1);
	lv_slider_set_range(speedSlider, 0, 100);
	lv_slider_bind_value(speedSlider, m_animSpeedBridge->getSubject());

	// --- Quality level ---
	lv_obj_t* qualBtn = add_list_btn(list, LV_SYMBOL_SETTINGS, "Quality");
	lv_obj_set_flex_grow(lv_obj_get_child(qualBtn, 1), 1);
	lv_obj_t* qualValBtn = lv_button_create(qualBtn);
	lv_obj_set_size(qualValBtn, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_t* qualLabel = lv_label_create(qualValBtn);

	// Show current quality as text
	auto qual_text = [](int32_t v) -> const char* {
		if (v <= 0) return "Low";
		if (v == 1) return "Medium";
		return "High";
	};
	lv_label_set_text(qualLabel, qual_text(lv_subject_get_int(m_qualityBridge->getSubject())));

	lv_subject_add_observer_obj(
		m_qualityBridge->getSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			lv_obj_t* lbl = lv_observer_get_target_obj(observer);
			if (!lbl) return;
			int32_t v = lv_subject_get_int(subject);
			if (v <= 0) lv_label_set_text(lbl, "Low");
			else if (v == 1) lv_label_set_text(lbl, "Medium");
			else lv_label_set_text(lbl, "High");
		},
		qualLabel, nullptr);

	lv_subject_increment_dsc_t* qual_dsc = lv_obj_add_subject_increment_event(
		qualValBtn, m_qualityBridge->getSubject(), LV_EVENT_CLICKED, 1);
	lv_obj_set_subject_increment_event_min_value(qualValBtn, qual_dsc, 0);
	lv_obj_set_subject_increment_event_max_value(qualValBtn, qual_dsc, 2);
	lv_obj_set_subject_increment_event_rollover(qualValBtn, qual_dsc, true);

	// --- Fallback status (informational, read-only) ---
	lv_list_add_text(list, "Performance");
	lv_obj_t* cpuBtn = add_list_btn(list, LV_SYMBOL_WARNING, "CPU Usage");
	lv_obj_set_flex_grow(lv_obj_get_child(cpuBtn, 1), 1);
	lv_obj_t* cpuLabel = lv_label_create(cpuBtn);

	// Bind CPU usage via a bridge stored as member so it is cleaned up with the page
	auto cpuBridgePtr = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(wm.getCpuUsageObservable());
	lv_label_bind_text(cpuLabel, cpuBridgePtr->getSubject(), "%ld%%");
	// Transfer ownership: store raw pointer as LVGL user data so it lives as long as the label
	// The bridge is kept alive by storing it in a static list cleaned up on destruction.
	// For simplicity, keep it in the unique_ptr released to a per-object member.
	m_cpuUsageBridge = std::move(cpuBridgePtr);

	lv_obj_add_flag(m_container, LV_OBJ_FLAG_HIDDEN);
}

void EffectsPage::show() {
	if (m_container != nullptr) {
		lv_obj_remove_flag(m_container, LV_OBJ_FLAG_HIDDEN);
	}
}

void EffectsPage::hide() {
	if (m_container != nullptr) {
		lv_obj_add_flag(m_container, LV_OBJ_FLAG_HIDDEN);
	}
}

} // namespace System::Apps::WallpaperEngine
