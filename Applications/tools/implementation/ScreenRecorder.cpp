#include "ScreenRecorder.hpp"

#include <Config.hpp>
#include <flx/system/services/ScreenRecorderService.hpp>
#include <flx/ui/common/SettingsCommon.hpp>
#include <flx/ui/theming/UiThemeManager.hpp>
#include <flx/ui/theming/layout_constants/LayoutConstants.hpp>
#include <flx/ui/theming/theme_engine/ThemeEngine.hpp>
#include <flx/ui/theming/themes/Themes.hpp>
#include <flx/ui/theming/ui_constants/UiConstants.hpp>
#include <sdkconfig.h>

#if FLXOS_SD_CARD_ENABLED
#include <flx/system/services/SdCardService.hpp>
#endif

#include <cstdio>

using namespace flx::ui::common;

namespace System::Apps::Tools {

void ScreenRecorder::createView(lv_obj_t* parent, std::function<void()> onBack) {
	m_view = create_page_container(parent);

	lv_obj_t* backBtn = nullptr;
	create_header(m_view, "Screen Recorder", &backBtn);

	m_onBack = onBack;
	add_back_button_event_cb(backBtn, &m_onBack);

	lv_obj_t* content = lv_obj_create(m_view);
	lv_obj_set_size(content, lv_pct(100), lv_pct(100));
	lv_obj_set_flex_grow(content, 1);
	lv_obj_set_style_border_width(content, 0, 0);
	lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_gap(content, lv_dpx(UiConstants::PAD_DEFAULT), 0);

	lv_obj_t* durationRow = lv_obj_create(content);
	lv_obj_remove_style_all(durationRow);
	lv_obj_set_size(durationRow, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(durationRow, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(durationRow, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_t* durationLabel = lv_label_create(durationRow);
	lv_label_set_text(durationLabel, "Duration:");
	lv_obj_set_style_min_width(durationLabel, lv_dpx(70), 0);

	m_durationSlider = lv_slider_create(durationRow);
	lv_slider_set_range(m_durationSlider, 5, 60);
	lv_slider_set_value(m_durationSlider, flx::services::ScreenRecorderService::getInstance().getDefaultDurationSec(), LV_ANIM_OFF);
	lv_obj_set_flex_grow(m_durationSlider, 1);

	m_durationValueLabel = lv_label_create(durationRow);
	lv_label_set_text_fmt(m_durationValueLabel, "%lus", static_cast<unsigned long>(lv_slider_get_value(m_durationSlider)));
	lv_obj_set_style_min_width(m_durationValueLabel, lv_dpx(42), 0);

	lv_obj_add_event_cb(
		m_durationSlider,
		[](lv_event_t* e) {
			auto* self = static_cast<ScreenRecorder*>(lv_event_get_user_data(e));
			uint32_t value = lv_slider_get_value(lv_event_get_target_obj(e));
			lv_label_set_text_fmt(self->m_durationValueLabel, "%lus", static_cast<unsigned long>(value));
		},
		LV_EVENT_VALUE_CHANGED, this);

	lv_obj_t* intervalRow = lv_obj_create(content);
	lv_obj_remove_style_all(intervalRow);
	lv_obj_set_size(intervalRow, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(intervalRow, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(intervalRow, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_t* intervalLabel = lv_label_create(intervalRow);
	lv_label_set_text(intervalLabel, "FPS:");
	lv_obj_set_style_min_width(intervalLabel, lv_dpx(70), 0);

	m_intervalSlider = lv_slider_create(intervalRow);
	lv_slider_set_range(m_intervalSlider, 1, 12);
	lv_slider_set_value(m_intervalSlider, 2, LV_ANIM_OFF);
	lv_obj_set_flex_grow(m_intervalSlider, 1);

	m_intervalValueLabel = lv_label_create(intervalRow);
	lv_label_set_text(m_intervalValueLabel, "2 fps");
	lv_obj_set_style_min_width(m_intervalValueLabel, lv_dpx(42), 0);

	lv_obj_add_event_cb(
		m_intervalSlider,
		[](lv_event_t* e) {
			auto* self = static_cast<ScreenRecorder*>(lv_event_get_user_data(e));
			uint32_t value = lv_slider_get_value(lv_event_get_target_obj(e));
			lv_label_set_text_fmt(self->m_intervalValueLabel, "%lu fps", static_cast<unsigned long>(value));
		},
		LV_EVENT_VALUE_CHANGED, this);

	lv_obj_t* pathRow = lv_obj_create(content);
	lv_obj_remove_style_all(pathRow);
	lv_obj_set_size(pathRow, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(pathRow, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(pathRow, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	lv_obj_t* pathLabel = lv_label_create(pathRow);
	lv_label_set_text(pathLabel, "Save to:");
	lv_obj_set_style_min_width(pathLabel, lv_dpx(70), 0);

	m_pathDropdown = lv_dropdown_create(pathRow);
	lv_obj_set_flex_grow(m_pathDropdown, 1);

	std::string options = "Internal Flash";
	int defaultSel = 0;
	std::string defaultPath = flx::services::ScreenRecorderService::getInstance().getDefaultStoragePath();

#if FLXOS_SD_CARD_ENABLED
	if (flx::services::SdCardService::getInstance().isMounted()) {
		options += "\nSD Card";
		if (defaultPath == flx::services::SdCardService::getInstance().getMountPoint()) {
			defaultSel = 1;
		}
	}
#endif

	lv_dropdown_set_options(m_pathDropdown, options.c_str());
	lv_dropdown_set_selected(m_pathDropdown, defaultSel);

	m_recordBtn = lv_button_create(content);
	lv_obj_set_size(m_recordBtn, lv_pct(80), lv_dpx(LayoutConstants::SIZE_TOUCH_TARGET));
	lv_obj_set_style_bg_opa(m_recordBtn, LV_OPA_COVER, 0);

	m_recordBtnLabel = lv_label_create(m_recordBtn);
	lv_label_set_text(m_recordBtnLabel, LV_SYMBOL_VIDEO " Start");
	lv_obj_center(m_recordBtnLabel);

	lv_obj_add_event_cb(
		m_recordBtn,
		[](lv_event_t* e) {
			auto* self = static_cast<ScreenRecorder*>(lv_event_get_user_data(e));
			self->toggleRecording();
		},
		LV_EVENT_CLICKED, this);

	m_statusLabel = lv_label_create(content);
	lv_label_set_long_mode(m_statusLabel, LV_LABEL_LONG_WRAP);
	lv_obj_set_width(m_statusLabel, lv_pct(100));
	updateStatus("Ready", false, true);
	updateControls();

	auto& uiTheme = flx::ui::theming::UiThemeManager::getInstance();
	lv_subject_add_observer_obj(
		uiTheme.getThemeSubject(),
		[](lv_observer_t* observer, lv_subject_t* subject) {
			LV_UNUSED(subject);
			auto* self = static_cast<ScreenRecorder*>(lv_observer_get_user_data(observer));
			if (self) {
				self->updateControls();
				self->applyStatusTheme();
			}
		},
		m_view, this);
}

void ScreenRecorder::toggleRecording() {
	if (flx::services::ScreenRecorderService::getInstance().isRecording()) {
		stopRecording();
	} else {
		startRecording();
	}
}

void ScreenRecorder::startRecording() {
	uint32_t durationSec = lv_slider_get_value(m_durationSlider);
	uint32_t intervalMs = getSelectedIntervalMs();
	std::string storagePath = getSelectedBasePath();

	bool started = flx::services::ScreenRecorderService::getInstance().startRecording(
		storagePath,
		durationSec,
		intervalMs,
		[this](const flx::services::ScreenRecordingStats& stats) {
			char buf[160];
			snprintf(buf,
				sizeof(buf),
				"Saved %lu frames: %s",
				static_cast<unsigned long>(stats.frameCount - stats.failedFrames),
				stats.recordingPath.c_str());
			updateStatus(buf, stats.failedFrames > 0);
			updateControls();
		});

	if (started) {
		m_lastShownFrames = 0;
		m_lastShownActive = true;
		updateStatus("Recording...", false, true);
	} else {
		updateStatus("Could not start recording", true);
	}
	updateControls();
}

void ScreenRecorder::stopRecording() {
	flx::services::ScreenRecorderService::getInstance().stopRecording();
	updateControls();
}

std::string ScreenRecorder::getSelectedBasePath() {
	uint32_t sel = lv_dropdown_get_selected(m_pathDropdown);

#if FLXOS_SD_CARD_ENABLED
	if (sel == 1 && flx::services::SdCardService::getInstance().isMounted()) {
		return flx::services::SdCardService::getInstance().getMountPoint();
	}
#endif

	(void)sel;
	return "/data";
}

uint32_t ScreenRecorder::getSelectedIntervalMs() const {
	uint32_t fps = static_cast<uint32_t>(lv_slider_get_value(m_intervalSlider));
	if (fps == 0) {
		fps = 1;
	}
	return 1000U / fps;
}

void ScreenRecorder::update() {
	if (!m_view || lv_obj_has_flag(m_view, LV_OBJ_FLAG_HIDDEN)) {
		return;
	}

	auto stats = flx::services::ScreenRecorderService::getInstance().getStats();
	if (stats.active != m_lastShownActive || stats.frameCount != m_lastShownFrames) {
		m_lastShownActive = stats.active;
		m_lastShownFrames = stats.frameCount;
		if (stats.active) {
			char buf[96];
			snprintf(buf,
				sizeof(buf),
				"Recording %lu/%lu frames",
				static_cast<unsigned long>(stats.frameCount),
				static_cast<unsigned long>(stats.maxFrames));
			updateStatus(buf, false, true);
		}
		updateControls();
	}
}

void ScreenRecorder::updateControls() {
	bool active = flx::services::ScreenRecorderService::getInstance().isRecording();

	if (m_recordBtnLabel) {
		lv_label_set_text(m_recordBtnLabel, active ? LV_SYMBOL_STOP " Stop" : LV_SYMBOL_VIDEO " Start");
		lv_obj_center(m_recordBtnLabel);
	}

	ThemeConfig const cfg = Themes::GetConfig(ThemeEngine::get_current_theme());
	if (m_recordBtn) {
		lv_obj_set_style_bg_color(m_recordBtn, active ? cfg.error : cfg.success, 0);
	}
	if (m_recordBtnLabel) {
		lv_obj_set_style_text_color(m_recordBtnLabel, cfg.on_primary, 0);
	}

	if (m_durationSlider) active ? lv_obj_add_state(m_durationSlider, LV_STATE_DISABLED) : lv_obj_clear_state(m_durationSlider, LV_STATE_DISABLED);
	if (m_intervalSlider) active ? lv_obj_add_state(m_intervalSlider, LV_STATE_DISABLED) : lv_obj_clear_state(m_intervalSlider, LV_STATE_DISABLED);
	if (m_pathDropdown) active ? lv_obj_add_state(m_pathDropdown, LV_STATE_DISABLED) : lv_obj_clear_state(m_pathDropdown, LV_STATE_DISABLED);
}

void ScreenRecorder::updateStatus(const char* msg, bool isError, bool isNeutral) {
	if (!m_statusLabel) return;
	m_statusTone = isError ? StatusTone::Error : (isNeutral ? StatusTone::Neutral : StatusTone::Success);
	lv_label_set_text(m_statusLabel, msg);
	applyStatusTheme();
}

void ScreenRecorder::applyStatusTheme() {
	if (!m_statusLabel) {
		return;
	}

	ThemeConfig const cfg = Themes::GetConfig(ThemeEngine::get_current_theme());
	if (m_statusTone == StatusTone::Error) {
		lv_obj_set_style_text_color(m_statusLabel, cfg.error, 0);
	} else if (m_statusTone == StatusTone::Success) {
		lv_obj_set_style_text_color(m_statusLabel, cfg.success, 0);
	} else {
		lv_obj_set_style_text_color(m_statusLabel, cfg.text_secondary, 0);
	}
}

void ScreenRecorder::show() {
	if (m_view) lv_obj_remove_flag(m_view, LV_OBJ_FLAG_HIDDEN);
	updateControls();
}

void ScreenRecorder::hide() {
	if (m_view) lv_obj_add_flag(m_view, LV_OBJ_FLAG_HIDDEN);
}

void ScreenRecorder::destroy() {
	if (flx::services::ScreenRecorderService::getInstance().isRecording()) {
		flx::services::ScreenRecorderService::getInstance().stopRecording();
	}

	if (m_view) {
		lv_obj_del(m_view);
		m_view = nullptr;
	}

	m_durationSlider = nullptr;
	m_durationValueLabel = nullptr;
	m_intervalSlider = nullptr;
	m_intervalValueLabel = nullptr;
	m_pathDropdown = nullptr;
	m_recordBtn = nullptr;
	m_recordBtnLabel = nullptr;
	m_statusLabel = nullptr;
}

} // namespace System::Apps::Tools
