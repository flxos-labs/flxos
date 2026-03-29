#include "PresetsPage.hpp"

#include "../widgets/WallpaperPreviewCard.hpp"
#include <flx/core/Logger.hpp>
#include <flx/system/managers/WallpaperManager.hpp>
#include <flx/ui/common/SettingsCommon.hpp>
#include <flx/ui/wallpaper/PresetLibrary.hpp>
#include <memory>
#include <vector>

using namespace flx::ui::common;

static constexpr const char* TAG_PP = "PresetsPage";

namespace System::Apps::WallpaperEngine {

PresetsPage::PresetsPage(lv_obj_t* parent, std::function<void()> onBack)
	: m_onBack(std::move(onBack)) {

	m_container = create_page_container(parent);

	// Header
	lv_obj_t* backBtn = nullptr;
	create_header(m_container, "Wallpaper Presets", &backBtn);
	add_back_button_event_cb(backBtn, &m_onBack);

	// Scrollable list
	lv_obj_t* list = lv_obj_create(m_container);
	lv_obj_set_width(list, lv_pct(100));
	lv_obj_set_flex_grow(list, 1);
	lv_obj_set_style_border_width(list, 0, 0);
	lv_obj_set_style_pad_all(list, 0, 0);
	lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);

	// Ensure PresetLibrary is populated
	auto& library = flx::ui::wallpaper::PresetLibrary::getInstance();
	if (library.isEmpty()) {
		library.loadPresets();
	}

	auto presets = library.listPresets();
	if (presets.empty()) {
		lv_obj_t* empty = lv_label_create(list);
		lv_label_set_text(empty, "No presets available");
	} else {
		for (const auto* preset : presets) {
			auto card = std::make_unique<WallpaperPreviewCard>(
				list, *preset,
				[this](const std::string& id) { applyPreset(id); });
			m_cards.push_back(std::move(card));
		}
	}

	lv_obj_add_flag(m_container, LV_OBJ_FLAG_HIDDEN);
}

void PresetsPage::show() {
	if (m_container != nullptr) {
		lv_obj_remove_flag(m_container, LV_OBJ_FLAG_HIDDEN);
	}
}

void PresetsPage::hide() {
	if (m_container != nullptr) {
		lv_obj_add_flag(m_container, LV_OBJ_FLAG_HIDDEN);
	}
}

void PresetsPage::applyPreset(const std::string& id) {
	auto& library = flx::ui::wallpaper::PresetLibrary::getInstance();
	auto& manager = flx::system::WallpaperManager::getInstance();
	if (!library.applyPreset(id, &manager)) {
		Log::warn(TAG_PP, "Failed to apply preset: %s", id.c_str());
	}
}

} // namespace System::Apps::WallpaperEngine
