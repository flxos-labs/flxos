#include "Screenshot.hpp"
#include "../../ui/theming/ui_constants/UiConstants.hpp"
#include "core/apps/settings/SettingsCommon.hpp"
#include <flx/core/Logger.hpp>
#include "core/services/filesystem/FileSystemService.hpp"
#include "core/services/screenshot/ScreenshotService.hpp"
#include "core/tasks/gui/GuiTask.hpp"

#include "sdkconfig.h"

#if defined(CONFIG_FLXOS_SD_CARD_ENABLED)
#include "core/services/storage/SdCardService.hpp"
#endif

#include <cstdio>
#include <cstring>
#include <ctime>
#include <sys/stat.h>

static constexpr std::string_view TAG = "Screenshot";

namespace System::Apps::Tools {

// ──────────────────────────────────────────────────────
// UI Creation
// ──────────────────────────────────────────────────────

void Screenshot::createView(lv_obj_t* parent, std::function<void()> onBack) {
	m_view = Settings::create_page_container(parent);

	lv_obj_t* backBtn = nullptr;
	Settings::create_header(m_view, "Screenshot", &backBtn);

	m_onBack = onBack;
	Settings::add_back_button_event_cb(backBtn, &m_onBack);

	// --- Content area ---
	lv_obj_t* content = lv_obj_create(m_view);
	lv_obj_set_size(content, lv_pct(100), lv_pct(100));
	lv_obj_set_flex_grow(content, 1);
	lv_obj_set_style_border_width(content, 0, 0);
	lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	// --- Delay slider row ---
	lv_obj_t* delayRow = lv_obj_create(content);
	lv_obj_remove_style_all(delayRow);
	lv_obj_set_size(delayRow, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(delayRow, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(delayRow, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_t* delayLabel = lv_label_create(delayRow);
	lv_label_set_text(delayLabel, "Delay:");
	lv_obj_set_style_min_width(delayLabel, lv_dpx(50), 0);

	m_delaySlider = lv_slider_create(delayRow);
	lv_slider_set_range(m_delaySlider, 0, 10);
	lv_slider_set_value(m_delaySlider, Services::ScreenshotService::getInstance().getDefaultDelay(), LV_ANIM_OFF);
	lv_obj_set_flex_grow(m_delaySlider, 1);

	m_delayValueLabel = lv_label_create(delayRow);
	lv_label_set_text_fmt(m_delayValueLabel, "%ds", (int)Services::ScreenshotService::getInstance().getDefaultDelay());
	lv_obj_set_style_min_width(m_delayValueLabel, lv_dpx(30), 0);

	lv_obj_add_event_cb(
		m_delaySlider,
		[](lv_event_t* e) {
			auto* self = static_cast<Screenshot*>(lv_event_get_user_data(e));
			int val = lv_slider_get_value(lv_event_get_target_obj(e));
			lv_label_set_text_fmt(self->m_delayValueLabel, "%ds", val);
		},
		LV_EVENT_VALUE_CHANGED, this
	);

	// --- Path chooser row ---
	lv_obj_t* pathRow = lv_obj_create(content);
	lv_obj_remove_style_all(pathRow);
	lv_obj_set_size(pathRow, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(pathRow, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(pathRow, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_t* pathLabel = lv_label_create(pathRow);
	lv_label_set_text(pathLabel, "Save to:");
	lv_obj_set_style_min_width(pathLabel, lv_dpx(50), 0);

	m_pathDropdown = lv_dropdown_create(pathRow);
	lv_obj_set_flex_grow(m_pathDropdown, 1);

	// Build dropdown options based on available storage
	std::string options = "Internal Flash";
	int defaultSel = 0;
	std::string defaultPath = Services::ScreenshotService::getInstance().getDefaultStoragePath();

#if defined(CONFIG_FLXOS_SD_CARD_ENABLED)
	if (Services::SdCardService::getInstance().isMounted()) {
		options += "\nSD Card";
		if (defaultPath == Services::SdCardService::getInstance().getMountPoint()) {
			defaultSel = 1;
		}
	}
#endif
	lv_dropdown_set_options(m_pathDropdown, options.c_str());
	lv_dropdown_set_selected(m_pathDropdown, defaultSel);

	// --- Capture button ---
	m_captureBtn = lv_button_create(content);
	lv_obj_set_size(m_captureBtn, lv_pct(80), LV_SIZE_CONTENT);
	lv_obj_set_style_size(m_captureBtn, lv_pct(80), LV_SIZE_CONTENT, 0);
	lv_obj_set_style_bg_color(m_captureBtn, lv_color_hex(0x4CAF50), 0);
	lv_obj_set_style_bg_opa(m_captureBtn, LV_OPA_COVER, 0);

	lv_obj_t* btnLabel = lv_label_create(m_captureBtn);
	lv_label_set_text_fmt(btnLabel, "%s  Capture", LV_SYMBOL_IMAGE);
	lv_obj_center(btnLabel);

	lv_obj_add_event_cb(
		m_captureBtn,
		[](lv_event_t* e) {
			auto* self = static_cast<Screenshot*>(lv_event_get_user_data(e));
			self->startCapture();
		},
		LV_EVENT_CLICKED, this
	);

	// --- Status label ---
	m_statusLabel = lv_label_create(content);
	lv_label_set_text(m_statusLabel, "Ready");
	lv_obj_set_style_text_color(m_statusLabel, lv_color_hex(0xAAAAAA), 0);
	lv_label_set_long_mode(m_statusLabel, LV_LABEL_LONG_WRAP);
	lv_obj_set_width(m_statusLabel, lv_pct(100));
}

// ──────────────────────────────────────────────────────
// Capture Flow
// ──────────────────────────────────────────────────────

void Screenshot::startCapture() {
	uint32_t delaySec = lv_slider_get_value(m_delaySlider);
	std::string storagePath = getSelectedBasePath();

	// Use service to schedule capture with completion callback
	Services::ScreenshotService::getInstance().scheduleCapture(
		delaySec,
		storagePath,
		[this](bool success, const std::string& path) {
			if (success) {
				std::string msg = "Saved: " + path;
				updateStatus(msg.c_str());
			} else {
				updateStatus("Save failed!", true);
			}
		}
	);

	// Update local UI
	if (delaySec > 0) {
		updateStatus("Capturing...");
	}
}

// ──────────────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────────────

std::string Screenshot::getSelectedBasePath() {
	uint32_t sel = lv_dropdown_get_selected(m_pathDropdown);

#if defined(CONFIG_FLXOS_SD_CARD_ENABLED)
	if (sel == 1 && Services::SdCardService::getInstance().isMounted()) {
		return Services::SdCardService::getInstance().getMountPoint();
	}
#endif

	// Default: internal flash data partition
	(void)sel;
	return "/data";
}

void Screenshot::updateStatus(const char* msg, bool isError) {
	if (!m_statusLabel) return;
	lv_label_set_text(m_statusLabel, msg);
	lv_obj_set_style_text_color(
		m_statusLabel,
		isError ? lv_color_hex(0xFF5252) : lv_color_hex(0x4CAF50),
		0
	);
}

// ──────────────────────────────────────────────────────
// View Lifecycle
// ──────────────────────────────────────────────────────

void Screenshot::show() {
	if (m_view) lv_obj_remove_flag(m_view, LV_OBJ_FLAG_HIDDEN);
}

void Screenshot::hide() {
	if (m_view) lv_obj_add_flag(m_view, LV_OBJ_FLAG_HIDDEN);
}

void Screenshot::destroy() {
	// Cancel any pending capture to prevent callback on destroyed object
	Services::ScreenshotService::getInstance().cancelCapture();

	if (m_view) {
		lv_obj_del(m_view);
		m_view = nullptr;
	}
	m_statusLabel = nullptr;
	m_delaySlider = nullptr;
	m_delayValueLabel = nullptr;
	m_pathDropdown = nullptr;
	m_captureBtn = nullptr;
}

} // namespace System::Apps::Tools
