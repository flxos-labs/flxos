#pragma once

#include <flx/hal/DeviceRegistry.hpp>
#include <flx/hal/IDevice.hpp>
#include <flx/hal/sdcard/ISdCardDevice.hpp>
#include <flx/system/managers/SettingsManager.hpp>
#include <lvgl.h>
#include <memory>
#include <string>

#include "settings/SettingsPageBase.hpp"

using namespace flx::ui::common;
using namespace flx::system;

namespace System::Apps::Settings {

class StorageSettings : public SettingsPageBase {
public:

	using SettingsPageBase::SettingsPageBase;

protected:

	void createUI() override {
		m_container = create_page_container(m_parent);

		lv_obj_t* backBtn = nullptr;
		create_header(m_container, "Storage", &backBtn);
		add_back_button_event_cb(backBtn, &m_onBack);

		m_list = create_settings_list(m_container);

		buildUi();
	}

	void onShow() override {
		refresh();
	}

	void onDestroy() override {
		m_mountStateLabel = nullptr;
		m_mountPathLabel = nullptr;
		m_fsTypeLabel = nullptr;
		m_totalLabel = nullptr;
		m_freeLabel = nullptr;
		m_mountBtn = nullptr;
		m_unmountBtn = nullptr;
	}

private:

	void buildUi() {
		// ── SD Card Section ──
		lv_list_add_text(m_list, "SD Card");

		// Mount state
		lv_obj_t* stateBtn = add_list_btn(m_list, LV_SYMBOL_DRIVE, "Status");
		lv_obj_set_flex_grow(lv_obj_get_child(stateBtn, 1), 1);
		m_mountStateLabel = lv_label_create(stateBtn);
		lv_label_set_text(m_mountStateLabel, "Unknown");

		// Mount path
		lv_obj_t* pathBtn = add_list_btn(m_list, LV_SYMBOL_DIRECTORY, "Mount Path");
		lv_obj_set_flex_grow(lv_obj_get_child(pathBtn, 1), 1);
		m_mountPathLabel = lv_label_create(pathBtn);
		lv_label_set_text(m_mountPathLabel, "--");

		// Filesystem type
		lv_obj_t* fsBtn = add_list_btn(m_list, LV_SYMBOL_FILE, "Filesystem");
		lv_obj_set_flex_grow(lv_obj_get_child(fsBtn, 1), 1);
		m_fsTypeLabel = lv_label_create(fsBtn);
		lv_label_set_text(m_fsTypeLabel, "--");

		// Total capacity
		lv_obj_t* totalBtn = add_list_btn(m_list, LV_SYMBOL_CHARGE, "Total");
		lv_obj_set_flex_grow(lv_obj_get_child(totalBtn, 1), 1);
		m_totalLabel = lv_label_create(totalBtn);
		lv_label_set_text(m_totalLabel, "--");

		// Free space
		lv_obj_t* freeBtn = add_list_btn(m_list, LV_SYMBOL_OK, "Free Space");
		lv_obj_set_flex_grow(lv_obj_get_child(freeBtn, 1), 1);
		m_freeLabel = lv_label_create(freeBtn);
		lv_label_set_text(m_freeLabel, "--");

		// ── SD Card Actions ──
		lv_list_add_text(m_list, "SD Card Actions");

		// Mount button
		m_mountBtn = lv_list_add_button(m_list, LV_SYMBOL_DRIVE, "Mount Card");
		lv_obj_add_event_cb(
			m_mountBtn,
			[](lv_event_t* e) {
				auto* instance = (StorageSettings*)lv_event_get_user_data(e);
				instance->mountCard();
			},
			LV_EVENT_CLICKED, this);

		// Unmount button
		m_unmountBtn = lv_list_add_button(m_list, LV_SYMBOL_CLOSE, "Unmount Card");
		lv_obj_add_event_cb(
			m_unmountBtn,
			[](lv_event_t* e) {
				auto* instance = (StorageSettings*)lv_event_get_user_data(e);
				instance->unmountCard();
			},
			LV_EVENT_CLICKED, this);

		// Refresh button
		lv_obj_t* refreshBtn = lv_list_add_button(m_list, LV_SYMBOL_REFRESH, "Refresh");
		lv_obj_add_event_cb(
			refreshBtn,
			[](lv_event_t* e) {
				auto* instance = (StorageSettings*)lv_event_get_user_data(e);
				instance->refresh();
			},
			LV_EVENT_CLICKED, this);

		// ── Config Actions ──
		lv_list_add_text(m_list, "System Configuration");

		lv_obj_t* saveBtn = lv_list_add_button(m_list, LV_SYMBOL_SAVE, "Save Settings");
		lv_obj_add_event_cb(
			saveBtn,
			[](lv_event_t* /*e*/) {
				SettingsManager::getInstance().triggerSave();
			},
			LV_EVENT_CLICKED, nullptr);

		lv_obj_t* loadBtn = lv_list_add_button(m_list, LV_SYMBOL_REFRESH, "Reload Settings");
		lv_obj_add_event_cb(
			loadBtn,
			[](lv_event_t* /*e*/) {
				SettingsManager::getInstance().loadSettings();
			},
			LV_EVENT_CLICKED, nullptr);
	}

	void refresh() {
		auto sdCard = flx::hal::DeviceRegistry::getInstance()
			.findFirst<flx::hal::sdcard::ISdCardDevice>(flx::hal::IDevice::Type::SdCard);

		if (!sdCard) {
			if (m_mountStateLabel) lv_label_set_text(m_mountStateLabel, "No SD card device");
			if (m_mountPathLabel) lv_label_set_text(m_mountPathLabel, "--");
			if (m_fsTypeLabel) lv_label_set_text(m_fsTypeLabel, "--");
			if (m_totalLabel) lv_label_set_text(m_totalLabel, "--");
			if (m_freeLabel) lv_label_set_text(m_freeLabel, "--");
			return;
		}

		// Mount state
		auto state = sdCard->getMountState();
		const char* stateStr = "Unknown";
		switch (state) {
			case flx::hal::sdcard::ISdCardDevice::MountState::Mounted:
				stateStr = "Mounted";
				break;
			case flx::hal::sdcard::ISdCardDevice::MountState::Unmounted:
				stateStr = "Unmounted";
				break;
			case flx::hal::sdcard::ISdCardDevice::MountState::Error:
				stateStr = "Error";
				break;
			default:
				break;
		}
		if (m_mountStateLabel) lv_label_set_text(m_mountStateLabel, stateStr);

		// Mount path
		if (m_mountPathLabel) {
			auto path = sdCard->getMountPath();
			lv_label_set_text(m_mountPathLabel, path.empty() ? "--" : path.c_str());
		}

		// Card info (only if mounted)
		if (sdCard->isMounted()) {
			flx::hal::sdcard::ISdCardDevice::CardInfo info;
			if (sdCard->getCardInfo(info)) {
				if (m_fsTypeLabel) {
					lv_label_set_text(m_fsTypeLabel, info.fsType.empty() ? "--" : info.fsType.c_str());
				}
				if (m_totalLabel) {
					lv_label_set_text(m_totalLabel, formatBytes(info.totalBytes).c_str());
				}
				if (m_freeLabel) {
					lv_label_set_text(m_freeLabel, formatBytes(info.freeBytes).c_str());
				}
			} else {
				if (m_fsTypeLabel) lv_label_set_text(m_fsTypeLabel, "--");
				if (m_totalLabel) lv_label_set_text(m_totalLabel, "--");
				if (m_freeLabel) lv_label_set_text(m_freeLabel, "--");
			}
		} else {
			if (m_fsTypeLabel) lv_label_set_text(m_fsTypeLabel, "--");
			if (m_totalLabel) lv_label_set_text(m_totalLabel, "--");
			if (m_freeLabel) lv_label_set_text(m_freeLabel, "--");
		}
	}

	void mountCard() {
		auto sdCard = flx::hal::DeviceRegistry::getInstance()
			.findFirst<flx::hal::sdcard::ISdCardDevice>(flx::hal::IDevice::Type::SdCard);
		if (!sdCard) {
			return;
		}
		sdCard->mount("/sdcard");
		refresh();
	}

	void unmountCard() {
		auto sdCard = flx::hal::DeviceRegistry::getInstance()
			.findFirst<flx::hal::sdcard::ISdCardDevice>(flx::hal::IDevice::Type::SdCard);
		if (!sdCard) {
			return;
		}
		sdCard->unmount();
		refresh();
	}

	static std::string formatBytes(uint64_t bytes) {
		char buf[32];
		if (bytes >= 1073741824ULL) { // >= 1 GB
			snprintf(buf, sizeof(buf), "%.2f GB", (double)bytes / 1073741824.0);
		} else if (bytes >= 1048576ULL) { // >= 1 MB
			snprintf(buf, sizeof(buf), "%.1f MB", (double)bytes / 1048576.0);
		} else if (bytes >= 1024ULL) {
			snprintf(buf, sizeof(buf), "%.0f KB", (double)bytes / 1024.0);
		} else {
			snprintf(buf, sizeof(buf), "%llu B", (unsigned long long)bytes);
		}
		return std::string(buf);
	}

	lv_obj_t* m_mountStateLabel = nullptr;
	lv_obj_t* m_mountPathLabel = nullptr;
	lv_obj_t* m_fsTypeLabel = nullptr;
	lv_obj_t* m_totalLabel = nullptr;
	lv_obj_t* m_freeLabel = nullptr;
	lv_obj_t* m_mountBtn = nullptr;
	lv_obj_t* m_unmountBtn = nullptr;
};

} // namespace System::Apps::Settings
