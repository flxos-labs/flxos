#include "EffectsPage.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <flx/system/managers/WallpaperManager.hpp>
#include <flx/ui/common/SettingsCommon.hpp>
#include <string>

using namespace flx::ui::common;

namespace System::Apps::WallpaperEngine {

namespace {

const char* toWallpaperTypeLabel(const std::string& type) {
	if (type == "animated" || type == "gif") {
		return "Animated GIF";
	}
	if (type == "lottie") {
		return "Lottie";
	}
	if (type == "dynamic") {
		return "Dynamic";
	}
	if (type == "adaptive") {
		return "Adaptive";
	}
	return "Static";
}

int32_t parseIntEffect(const std::string& effects, const std::string& key, int32_t fallback) {
	std::string const needle = "\"" + key + "\":";
	size_t const pos = effects.find(needle);
	if (pos == std::string::npos) {
		return fallback;
	}

	char* end_ptr = nullptr;
	long const parsed = std::strtol(effects.c_str() + pos + needle.size(), &end_ptr, 10);
	if (end_ptr == nullptr || end_ptr == effects.c_str() + pos + needle.size()) {
		return fallback;
	}
	return static_cast<int32_t>(parsed);
}

float parseFloatEffect(const std::string& effects, const std::string& key, float fallback) {
	std::string const needle = "\"" + key + "\":";
	size_t const pos = effects.find(needle);
	if (pos == std::string::npos) {
		return fallback;
	}

	char* end_ptr = nullptr;
	float const parsed = std::strtof(effects.c_str() + pos + needle.size(), &end_ptr);
	if (end_ptr == nullptr || end_ptr == effects.c_str() + pos + needle.size()) {
		return fallback;
	}
	return parsed;
}

bool hasTrueEffect(const std::string& effects, const std::string& key) {
	std::string const needle = "\"" + key + "\":true";
	return effects.find(needle) != std::string::npos;
}

} // namespace

EffectsPage::EffectsPage(lv_obj_t* parent, std::function<void()> onBack)
	: m_parent(parent), m_onBack(std::move(onBack)) {

	auto& wm = flx::system::WallpaperManager::getInstance();
	std::string const currentEffects = wm.getWallpaperEffectsObservable().get();

	m_wpEnabledBridge = std::make_unique<flx::ui::LvglObserverBridge<int32_t>>(
		wm.getWallpaperEnabledObservable());
	m_wpSourceBridge = std::make_unique<flx::ui::LvglStringObserverBridge>(
		wm.getWallpaperSourceObservable());
	m_wpTypeBridge = std::make_unique<flx::ui::LvglStringObserverBridge>(
		wm.getWallpaperTypeObservable());
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

	// --- Wallpaper source picker ---
	lv_obj_t* chooseWpBtn = add_list_btn(list, LV_SYMBOL_DIRECTORY, "Choose Wallpaper");
	lv_obj_set_flex_grow(lv_obj_get_child(chooseWpBtn, 1), 1);
	lv_obj_t* wpSourceLabel = lv_label_create(chooseWpBtn);
	lv_label_set_text(wpSourceLabel, "None");

	lv_subject_add_observer_obj(
		m_wpSourceBridge->getSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			lv_obj_t* label = lv_observer_get_target_obj(observer);
			if (label == nullptr) {
				return;
			}

			const char* path = static_cast<const char*>(lv_subject_get_pointer(subject));
			if (path == nullptr || path[0] == '\0') {
				lv_label_set_text(label, "None");
				return;
			}

			std::string p = path;
			size_t const pos = p.find_last_of("/\\");
			std::string const filename = (pos == std::string::npos) ? p : p.substr(pos + 1);
			lv_label_set_text(label, filename.c_str());
		},
		wpSourceLabel,
		nullptr);

	lv_obj_add_event_cb(
		chooseWpBtn,
		[](lv_event_t* e) {
			auto* self = static_cast<EffectsPage*>(lv_event_get_user_data(e));
			if (self == nullptr) {
				return;
			}

			auto& manager = flx::system::WallpaperManager::getInstance();
			std::string const type = manager.getWallpaperTypeObservable().get();

			if (self->m_fileBrowser == nullptr) {
				self->m_fileBrowser = new flx::ui::FileBrowser(self->m_parent, [self]() {
					if (self->m_fileBrowser != nullptr) {
						self->m_fileBrowser->hide();
					}
				});
			}

			if (type == "animated" || type == "gif") {
				self->m_fileBrowser->setExtensions({".gif"});
			} else if (type == "lottie") {
				self->m_fileBrowser->setExtensions({".json"});
			} else {
				self->m_fileBrowser->setExtensions({".png", ".jpg", ".jpeg", ".bmp", ".webp"});
			}

			self->m_fileBrowser->show(false, [self](const std::string& path) {
				auto& managerInner = flx::system::WallpaperManager::getInstance();
				managerInner.setWallpaper(path, managerInner.getWallpaperTypeObservable().get());
				if (self->m_fileBrowser != nullptr) {
					self->m_fileBrowser->hide();
				}
			});
		},
		LV_EVENT_CLICKED,
		this);

	// --- Wallpaper type selector ---
	lv_obj_t* wpTypeBtn = add_list_btn(list, LV_SYMBOL_LIST, "Wallpaper Type");
	lv_obj_set_flex_grow(lv_obj_get_child(wpTypeBtn, 1), 1);
	lv_obj_t* wpTypeLabel = lv_label_create(wpTypeBtn);
	lv_label_set_text(wpTypeLabel, toWallpaperTypeLabel(wm.getWallpaperTypeObservable().get()));

	lv_subject_add_observer_obj(
		m_wpTypeBridge->getSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			lv_obj_t* label = lv_observer_get_target_obj(observer);
			if (label == nullptr) {
				return;
			}
			const char* type = static_cast<const char*>(lv_subject_get_pointer(subject));
			lv_label_set_text(label, toWallpaperTypeLabel(type == nullptr ? "static" : type));
		},
		wpTypeLabel,
		nullptr);

	lv_obj_add_event_cb(
		wpTypeBtn,
		[](lv_event_t* /*e*/) {
			auto& manager = flx::system::WallpaperManager::getInstance();
			std::string currentType = manager.getWallpaperTypeObservable().get();
			std::string nextType = "static";
			if (currentType == "static") {
				nextType = "animated";
			} else if (currentType == "animated" || currentType == "gif") {
				nextType = "lottie";
			}

			manager.setWallpaper(manager.getWallpaperSourceObservable().get(), nextType);
		},
		LV_EVENT_CLICKED,
		nullptr);

	// --- Animation speed ---
	lv_obj_t* speedBtn = add_list_btn(list, LV_SYMBOL_PLAY, "Animation Speed");
	lv_obj_t* speedSlider = lv_slider_create(speedBtn);
	lv_obj_set_flex_grow(speedSlider, 1);
	lv_slider_set_range(speedSlider, 0, 100);
	lv_slider_bind_value(speedSlider, m_animSpeedBridge->getSubject());

	// --- Blur effect ---
	lv_obj_t* blurBtn = add_list_btn(list, LV_SYMBOL_EDIT, "Blur");
	lv_obj_t* blurSlider = lv_slider_create(blurBtn);
	lv_obj_set_flex_grow(blurSlider, 1);
	lv_slider_set_range(blurSlider, 0, 20);
	lv_slider_set_value(blurSlider, parseIntEffect(currentEffects, "blur", 0), LV_ANIM_OFF);
	lv_obj_add_event_cb(
		blurSlider,
		[](lv_event_t* e) {
			auto* slider = lv_event_get_target_obj(e);
			int32_t const blur = lv_slider_get_value(slider);
			auto& manager = flx::system::WallpaperManager::getInstance();
			if (blur <= 0) {
				manager.removeEffect("blur");
			} else {
				manager.applyEffect("blur", std::to_string(blur));
			}
		},
		LV_EVENT_VALUE_CHANGED,
		nullptr);

	// --- Brightness effect ---
	lv_obj_t* brightnessBtn = add_list_btn(list, LV_SYMBOL_EYE_OPEN, "Brightness");
	lv_obj_t* brightnessSlider = lv_slider_create(brightnessBtn);
	lv_obj_set_flex_grow(brightnessSlider, 1);
	lv_slider_set_range(brightnessSlider, 50, 150);
	float const brightnessValue = parseFloatEffect(currentEffects, "brightness", 1.0f);
	int32_t const brightnessPercent = std::clamp(
		static_cast<int32_t>(brightnessValue * 100.0f),
		static_cast<int32_t>(50),
		static_cast<int32_t>(150));
	lv_slider_set_value(brightnessSlider, brightnessPercent, LV_ANIM_OFF);
	lv_obj_add_event_cb(
		brightnessSlider,
		[](lv_event_t* e) {
			auto* slider = lv_event_get_target_obj(e);
			int32_t const percent = lv_slider_get_value(slider);
			auto& manager = flx::system::WallpaperManager::getInstance();
			if (percent == 100) {
				manager.removeEffect("brightness");
				return;
			}

			char valueBuf[16];
			std::snprintf(
				valueBuf,
				sizeof(valueBuf),
				"%.2f",
				static_cast<double>(static_cast<float>(percent) / 100.0f));
			manager.applyEffect("brightness", valueBuf);
		},
		LV_EVENT_VALUE_CHANGED,
		nullptr);

	// --- Transition effect ---
	lv_obj_t* transitionBtn = add_list_btn(list, LV_SYMBOL_REFRESH, "Transitions");
	lv_obj_set_flex_grow(lv_obj_get_child(transitionBtn, 1), 1);
	lv_obj_t* transitionSw = lv_switch_create(transitionBtn);
	if (hasTrueEffect(currentEffects, "fade_transition")) {
		lv_obj_add_state(transitionSw, LV_STATE_CHECKED);
	}
	lv_obj_add_event_cb(
		transitionSw,
		[](lv_event_t* e) {
			auto* sw = lv_event_get_target_obj(e);
			auto& manager = flx::system::WallpaperManager::getInstance();
			if (lv_obj_has_state(sw, LV_STATE_CHECKED)) {
				manager.applyEffect("fade_transition", "true");
			} else {
				manager.removeEffect("fade_transition");
			}
		},
		LV_EVENT_VALUE_CHANGED,
		nullptr);

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
			else if (v == 1)
				lv_label_set_text(lbl, "Medium");
			else
				lv_label_set_text(lbl, "High");
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

EffectsPage::~EffectsPage() {
	if (m_fileBrowser != nullptr) {
		delete m_fileBrowser;
		m_fileBrowser = nullptr;
	}
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
