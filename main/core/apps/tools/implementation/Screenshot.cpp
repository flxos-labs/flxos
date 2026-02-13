#include "Screenshot.hpp"
#include "../../ui/theming/ui_constants/UiConstants.hpp"
#include "core/apps/settings/SettingsCommon.hpp"
#include "core/common/Logger.hpp"
#include "core/services/filesystem/FileSystemService.hpp"
#include "core/services/screenshot/ScreenshotService.hpp"
#include "core/tasks/gui/GuiTask.hpp"
#include "core/ui/desktop/modules/status_bar/StatusBar.hpp"
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
	lv_slider_set_value(m_delaySlider, 3, LV_ANIM_OFF);
	lv_obj_set_flex_grow(m_delaySlider, 1);

	m_delayValueLabel = lv_label_create(delayRow);
	lv_label_set_text(m_delayValueLabel, "3s");
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
#if defined(CONFIG_FLXOS_SD_CARD_ENABLED)
	if (Services::SdCardService::getInstance().isMounted()) {
		options += "\nSD Card";
	}
#endif
	lv_dropdown_set_options(m_pathDropdown, options.c_str());

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
	// Don't start if already counting down
	if (m_countdownTimer) return;

	int delay = lv_slider_get_value(m_delaySlider);

	if (delay == 0) {
		// Instant capture
		updateStatus("Capturing...");
		doCapture();
		return;
	}

	// Start countdown
	m_countdownRemaining = delay;
	lv_obj_add_state(m_captureBtn, LV_STATE_DISABLED);

	// Show first countdown tick in status bar
	char buf[16];
	snprintf(buf, sizeof(buf), LV_SYMBOL_IMAGE " %d", m_countdownRemaining);
	UI::Modules::StatusBar::showOverlay(buf);
	updateStatus("Capturing...");

	m_countdownTimer = lv_timer_create(
		[](lv_timer_t* t) {
			auto* self = static_cast<Screenshot*>(lv_timer_get_user_data(t));
			self->onCountdownTick();
		},
		1000, this
	);
}

void Screenshot::onCountdownTick() {
	m_countdownRemaining--;

	if (m_countdownRemaining <= 0) {
		// Stop countdown timer
		lv_timer_delete(m_countdownTimer);
		m_countdownTimer = nullptr;

		UI::Modules::StatusBar::clearOverlay();
		lv_obj_clear_state(m_captureBtn, LV_STATE_DISABLED);

		doCapture();
	} else {
		// Update status bar overlay
		char buf[16];
		snprintf(buf, sizeof(buf), LV_SYMBOL_IMAGE " %d", m_countdownRemaining);
		UI::Modules::StatusBar::showOverlay(buf);
	}
}

bool Screenshot::doCapture() {
	auto& svc = Services::ScreenshotService::getInstance();

	std::string path = generateFilename();
	if (path.empty()) {
		updateStatus("Save failed: storage not available", true);
		return false;
	}

	bool ok = svc.capture(path);

	if (ok) {
		Log::info(TAG, "Screenshot saved: %s", path.c_str());
		std::string msg = "Saved: " + path;
		updateStatus(msg.c_str());
	} else {
		Log::error(TAG, "Failed to save screenshot: %s", path.c_str());
		updateStatus("Save failed!", true);
	}
	return ok;
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

std::string Screenshot::generateFilename() {
	std::string base = getSelectedBasePath();
	std::string dir = base + "/screenshots";

	// Create directory if it doesn't exist
	Services::FileSystemService::getInstance().mkdir(dir);

	// Try to use RTC time for filename
	time_t now = 0;
	time(&now);
	struct tm timeinfo = {};
	localtime_r(&now, &timeinfo);

	char filename[64];
	if (timeinfo.tm_year > 100) { // Year > 2000 means RTC is set
		snprintf(filename, sizeof(filename), "/scr_%04d%02d%02d_%02d%02d%02d.png", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
	} else {
		// Fallback: find next available number
		bool found = false;
		for (int i = 1; i <= 99999; i++) {
			snprintf(filename, sizeof(filename), "/scr_%05d.png", i);
			std::string full = dir + filename;
			struct stat st {};
			if (stat(full.c_str(), &st) != 0) {
				found = true;
				break; // File doesn't exist, use this name
			}
		}
		if (!found) {
			Log::error(TAG, "All screenshot filename slots exhausted");
			return {};
		}
	}

	return dir + filename;
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
	if (m_countdownTimer) {
		lv_timer_delete(m_countdownTimer);
		m_countdownTimer = nullptr;
		UI::Modules::StatusBar::clearOverlay();
	}
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
