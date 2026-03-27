#include "DynamicPage.hpp"

#include <flx/system/managers/WallpaperManager.hpp>
#include <flx/ui/common/SettingsCommon.hpp>

using namespace flx::ui::common;

namespace System::Apps::WallpaperEngine {

struct AlgorithmEntry {
	const char* label;
	const char* source;
	const char* description;
};

static constexpr AlgorithmEntry ALGORITHMS[] = {
	{"Plasma", "algo://plasma", "Vibrant sinusoidal colour interference"},
	{"Cloud Noise", "algo://perlin", "Smooth organic noise cloud pattern"},
	{"Gradient Waves", "algo://gradient", "Animated colour gradient waves"},
};

DynamicPage::DynamicPage(lv_obj_t* parent, std::function<void()> onBack)
	: m_onBack(std::move(onBack)) {

	m_container = create_page_container(parent);

	// Header
	lv_obj_t* backBtn = nullptr;
	create_header(m_container, "Dynamic Wallpapers", &backBtn);
	add_back_button_event_cb(backBtn, &m_onBack);

	lv_obj_t* list = create_settings_list(m_container);
	lv_list_add_text(list, "Select Algorithm");

	for (const auto& algo : ALGORITHMS) {
		lv_obj_t* btn = add_list_btn(list, LV_SYMBOL_LOOP, algo.label);
		lv_obj_set_flex_grow(lv_obj_get_child(btn, 1), 1);

		lv_obj_t* descLabel = lv_label_create(btn);
		lv_label_set_text(descLabel, algo.description);
		lv_obj_set_style_text_opa(descLabel, LV_OPA_60, 0);

		// Store source pointer directly — string literals have static storage duration
		lv_obj_set_user_data(btn, static_cast<void*>(const_cast<char*>(algo.source)));

		lv_obj_add_event_cb(
			btn,
			[](lv_event_t* e) {
				const char* source = static_cast<const char*>(
					lv_obj_get_user_data(lv_event_get_target_obj(e)));
				if (source != nullptr) {
					flx::system::WallpaperManager::getInstance().setWallpaper(
						source, "dynamic");
				}
			},
			LV_EVENT_CLICKED, nullptr);
	}

	lv_obj_add_flag(m_container, LV_OBJ_FLAG_HIDDEN);
}

void DynamicPage::show() {
	if (m_container != nullptr) {
		lv_obj_remove_flag(m_container, LV_OBJ_FLAG_HIDDEN);
	}
}

void DynamicPage::hide() {
	if (m_container != nullptr) {
		lv_obj_add_flag(m_container, LV_OBJ_FLAG_HIDDEN);
	}
}

} // namespace System::Apps::WallpaperEngine
