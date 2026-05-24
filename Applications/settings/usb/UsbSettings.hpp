#pragma once

#include <flx/hal/DeviceRegistry.hpp>
#include <flx/hal/IDevice.hpp>
#include <flx/hal/usb/IUsbDevice.hpp>
#include <lvgl.h>
#include <memory>

#include "settings/SettingsPageBase.hpp"

using namespace flx::ui::common;

namespace System::Apps::Settings {

class UsbSettings : public SettingsPageBase {
public:

	using SettingsPageBase::SettingsPageBase;

protected:

	void createUI() override {
		m_container = create_page_container(m_parent);

		lv_obj_t* backBtn = nullptr;
		create_header(m_container, "USB", &backBtn);
		add_back_button_event_cb(backBtn, &m_onBack);

		m_list = create_settings_list(m_container);

		buildUi();
	}

	void onShow() override {
		refresh();
	}

	void onDestroy() override {
		m_currentModeLabel = nullptr;
		m_modeDropdown = nullptr;
		m_applyBtn = nullptr;
		m_rebootBtn = nullptr;
	}

private:

	void buildUi() {
		auto usb = getUsbDevice();

		// ── Current Status ──
		lv_list_add_text(m_list, "USB Status");

		lv_obj_t* statusBtn = add_list_btn(m_list, LV_SYMBOL_USB, "Current Mode");
		lv_obj_set_flex_grow(lv_obj_get_child(statusBtn, 1), 1);
		m_currentModeLabel = lv_label_create(statusBtn);

		if (!usb) {
			lv_label_set_text(m_currentModeLabel, "No USB device");
		} else if (!usb->isSupported()) {
			lv_label_set_text(m_currentModeLabel, "Not Supported");
		} else {
			lv_label_set_text(m_currentModeLabel, modeToString(usb->getCurrentMode()));
		}

		// ── Mode Selection ──
		lv_list_add_text(m_list, "USB Mode");

		lv_obj_t* modeBtn = add_list_btn(m_list, LV_SYMBOL_USB, "Select Mode");
		lv_obj_set_flex_grow(lv_obj_get_child(modeBtn, 1), 1);

		m_modeDropdown = lv_dropdown_create(modeBtn);
		lv_dropdown_set_options(
			m_modeDropdown,
			"Default (Profile)\n"
			"None (Disabled)\n"
			"Mass Storage — SD Card\n"
			"Mass Storage — Flash\n"
			"CDC Serial");
		lv_dropdown_set_dir(m_modeDropdown, LV_DIR_LEFT);
		lv_obj_set_width(m_modeDropdown, LV_SIZE_CONTENT);

		// Pre-select current mode
		if (usb && usb->isSupported()) {
			lv_dropdown_set_selected(m_modeDropdown, modeToIndex(usb->getCurrentMode()));
		}

		// Update reboot notice when mode selection changes
		lv_obj_add_event_cb(
			m_modeDropdown,
			[](lv_event_t* e) {
				auto* instance = (UsbSettings*)lv_event_get_user_data(e);
				instance->onModeSelectionChanged();
			},
			LV_EVENT_VALUE_CHANGED, this);

		// ── Actions ──
		lv_list_add_text(m_list, "Actions");

		// Reboot notice (hidden by default)
		m_rebootNotice = lv_list_add_button(m_list, LV_SYMBOL_WARNING, "Reboot required for this mode");
		lv_obj_set_style_text_opa(lv_obj_get_child(m_rebootNotice, 1), LV_OPA_70, 0);
		lv_obj_add_flag(m_rebootNotice, LV_OBJ_FLAG_HIDDEN);

		// Apply button
		m_applyBtn = lv_list_add_button(m_list, LV_SYMBOL_OK, "Apply Mode");
		lv_obj_add_event_cb(
			m_applyBtn,
			[](lv_event_t* e) {
				auto* instance = (UsbSettings*)lv_event_get_user_data(e);
				instance->applyMode(false);
			},
			LV_EVENT_CLICKED, this);

		// Reboot & apply button (initially hidden)
		m_rebootBtn = lv_list_add_button(m_list, LV_SYMBOL_POWER, "Reboot & Apply Mode");
		lv_obj_add_flag(m_rebootBtn, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_event_cb(
			m_rebootBtn,
			[](lv_event_t* e) {
				auto* instance = (UsbSettings*)lv_event_get_user_data(e);
				instance->applyMode(true);
			},
			LV_EVENT_CLICKED, this);

		// Disable all controls if USB not supported or not found
		if (!usb || !usb->isSupported()) {
			if (m_modeDropdown) lv_obj_add_state(m_modeDropdown, LV_STATE_DISABLED);
			if (m_applyBtn) lv_obj_add_state(m_applyBtn, LV_STATE_DISABLED);
		}

		onModeSelectionChanged();
	}

	void refresh() {
		auto usb = getUsbDevice();
		if (!usb || !m_currentModeLabel) {
			return;
		}
		lv_label_set_text(m_currentModeLabel, usb->isSupported() ? modeToString(usb->getCurrentMode()) : "Not Supported");
	}

	void onModeSelectionChanged() {
		auto usb = getUsbDevice();
		if (!usb || !m_modeDropdown) {
			return;
		}
		uint32_t const sel = lv_dropdown_get_selected(m_modeDropdown);
		auto const mode = indexToMode(sel);
		bool const canRebootIntoMode = usb->isSupported() && usb->canReboot(mode);

		if (m_rebootNotice) {
			if (canRebootIntoMode) {
				if (auto* label = lv_obj_get_child(m_rebootNotice, 1)) {
					lv_label_set_text(label, "Reboot option available for this mode");
				}
				lv_obj_remove_flag(m_rebootNotice, LV_OBJ_FLAG_HIDDEN);
			} else {
				lv_obj_add_flag(m_rebootNotice, LV_OBJ_FLAG_HIDDEN);
			}
		}
		if (m_rebootBtn) {
			if (canRebootIntoMode) {
				lv_obj_remove_flag(m_rebootBtn, LV_OBJ_FLAG_HIDDEN);
			} else {
				lv_obj_add_flag(m_rebootBtn, LV_OBJ_FLAG_HIDDEN);
			}
		}
		if (m_applyBtn && usb->isSupported()) {
			if (needsReboot) {
				lv_obj_add_state(m_applyBtn, LV_STATE_DISABLED);
			} else {
				lv_obj_remove_state(m_applyBtn, LV_STATE_DISABLED);
			}
		}
	}

	void applyMode(bool reboot) {
		auto usb = getUsbDevice();
		if (!usb || !usb->isSupported() || !m_modeDropdown) {
			return;
		}

		uint32_t const sel = lv_dropdown_get_selected(m_modeDropdown);
		auto const mode = indexToMode(sel);

		if (mode == flx::hal::usb::IUsbDevice::UsbMode::Default) {
			if (reboot) {
				usb->rebootInto(mode);
			}
			return;
		}

		if (reboot) {
			usb->rebootInto(mode);
			return;
		}

		switch (mode) {
			case flx::hal::usb::IUsbDevice::UsbMode::None:
				usb->stopMode();
				break;
			case flx::hal::usb::IUsbDevice::UsbMode::MassStorageSdCard:
			case flx::hal::usb::IUsbDevice::UsbMode::MassStorageFlash:
				usb->startMassStorage(mode);
				break;
			case flx::hal::usb::IUsbDevice::UsbMode::CdcSerial:
				usb->startCdcSerial();
				break;
			default:
				break;
		}

		refresh();
	}

	static std::shared_ptr<flx::hal::usb::IUsbDevice> getUsbDevice() {
		return flx::hal::DeviceRegistry::getInstance()
			.findFirst<flx::hal::usb::IUsbDevice>(flx::hal::IDevice::Type::Usb);
	}

	static const char* modeToString(flx::hal::usb::IUsbDevice::UsbMode mode) {
		using UsbMode = flx::hal::usb::IUsbDevice::UsbMode;
		switch (mode) {
			case UsbMode::None:
				return "Disabled";
			case UsbMode::Default:
				return "Default";
			case UsbMode::MassStorageSdCard:
				return "Mass Storage (SD)";
			case UsbMode::MassStorageFlash:
				return "Mass Storage (Flash)";
			case UsbMode::CdcSerial:
				return "CDC Serial";
		}
		return "Unknown";
	}

	static uint32_t modeToIndex(flx::hal::usb::IUsbDevice::UsbMode mode) {
		using UsbMode = flx::hal::usb::IUsbDevice::UsbMode;
		switch (mode) {
			case UsbMode::Default:
				return 0;
			case UsbMode::None:
				return 1;
			case UsbMode::MassStorageSdCard:
				return 2;
			case UsbMode::MassStorageFlash:
				return 3;
			case UsbMode::CdcSerial:
				return 4;
			default:
				return 0;
		}
	}

	static flx::hal::usb::IUsbDevice::UsbMode indexToMode(uint32_t index) {
		using UsbMode = flx::hal::usb::IUsbDevice::UsbMode;
		switch (index) {
			case 0:
				return UsbMode::Default;
			case 1:
				return UsbMode::None;
			case 2:
				return UsbMode::MassStorageSdCard;
			case 3:
				return UsbMode::MassStorageFlash;
			case 4:
				return UsbMode::CdcSerial;
			default:
				return UsbMode::Default;
		}
	}

	lv_obj_t* m_currentModeLabel = nullptr;
	lv_obj_t* m_modeDropdown = nullptr;
	lv_obj_t* m_applyBtn = nullptr;
	lv_obj_t* m_rebootBtn = nullptr;
	lv_obj_t* m_rebootNotice = nullptr;
};

} // namespace System::Apps::Settings
