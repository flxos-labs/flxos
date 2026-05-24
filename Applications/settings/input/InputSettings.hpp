#pragma once

#include <cstdint>
#include <flx/hal/DeviceRegistry.hpp>
#include <flx/hal/IDevice.hpp>
#include <flx/hal/input/IInputDevice.hpp>
#include <lvgl.h>
#include <memory>
#include <string>
#include <vector>

#include "settings/SettingsPageBase.hpp"

using namespace flx::ui::common;

namespace System::Apps::Settings {

class InputSettings : public SettingsPageBase {
public:

	using SettingsPageBase::SettingsPageBase;

protected:

	void createUI() override {
		m_container = create_page_container(m_parent);

		lv_obj_t* backBtn = nullptr;
		create_header(m_container, "Input Devices", &backBtn);
		add_back_button_event_cb(backBtn, &m_onBack);

		m_list = create_settings_list(m_container);

		buildUi();
	}

	void onShow() override {
		refreshDeviceList();
	}

	void onHide() override {
		unsubscribeKeyEvents();
	}

	void onDestroy() override {
		unsubscribeKeyEvents();
		m_keyEventLabel = nullptr;
		m_keyCodeLabel = nullptr;
		m_devicesSection = nullptr;
	}

private:

	void buildUi() {
		// ── Device list section ──
		lv_list_add_text(m_list, "Registered Input Devices");
		m_devicesSection = m_list; // Devices will be added here

		refreshDeviceList();
	}

	void refreshDeviceList() {
		if (!m_list) {
			return;
		}

		// Remove old device items (clean the device entries from list)
		// We use a simple rebuild: wipe the list and rebuild everything
		lv_obj_clean(m_list);

		lv_list_add_text(m_list, "Registered Input Devices");

		auto& reg = flx::hal::DeviceRegistry::getInstance();

		// Query all keyboard and encoder devices
		auto keyboards = reg.findAll<flx::hal::input::IInputDevice>(flx::hal::IDevice::Type::Keyboard);
		auto encoders = reg.findAll<flx::hal::input::IInputDevice>(flx::hal::IDevice::Type::Encoder);

		bool hasDevices = false;

		// Show keyboards
		for (const auto& dev: keyboards) {
			addDeviceEntry(dev, "Keyboard");
			hasDevices = true;
		}

		// Show encoders
		for (const auto& dev: encoders) {
			addDeviceEntry(dev, "Encoder");
			hasDevices = true;
		}

		if (!hasDevices) {
			lv_list_add_text(m_list, "No input devices registered");
		}

		// ── Key Test Panel ──
		lv_list_add_text(m_list, "Key Test Panel");

		lv_obj_t* testInfoBtn = add_list_btn(m_list, LV_SYMBOL_KEYBOARD, "Last Key Event");
		lv_obj_set_flex_grow(lv_obj_get_child(testInfoBtn, 1), 1);
		m_keyEventLabel = lv_label_create(testInfoBtn);
		lv_label_set_text(m_keyEventLabel, "Press a key...");

		lv_obj_t* keyCodeBtn = add_list_btn(m_list, LV_SYMBOL_OK, "Key Code");
		lv_obj_set_flex_grow(lv_obj_get_child(keyCodeBtn, 1), 1);
		m_keyCodeLabel = lv_label_create(keyCodeBtn);
		lv_label_set_text(m_keyCodeLabel, "--");

		// Re-subscribe after rebuild
		unsubscribeKeyEvents();
		subscribeKeyEvents();
	}

	void addDeviceEntry(const std::shared_ptr<flx::hal::input::IInputDevice>& dev, const char* typeLabel) {
		std::string name(dev->getName());
		auto state = dev->getState();

		lv_obj_t* btn = add_list_btn(m_list, LV_SYMBOL_KEYBOARD, name.c_str());
		lv_obj_set_flex_grow(lv_obj_get_child(btn, 1), 1);

		// Type & state label
		lv_obj_t* stateLabel = lv_label_create(btn);
		char buf[64];
		snprintf(buf, sizeof(buf), "%s | %s",
			typeLabel,
			flx::hal::IDevice::stateToString(state));
		lv_label_set_text(stateLabel, buf);
		lv_obj_set_style_text_opa(stateLabel, LV_OPA_70, 0);
	}

	void subscribeKeyEvents() {
		auto& reg = flx::hal::DeviceRegistry::getInstance();
		auto keyboards = reg.findAll<flx::hal::input::IInputDevice>(flx::hal::IDevice::Type::Keyboard);
		auto encoders = reg.findAll<flx::hal::input::IInputDevice>(flx::hal::IDevice::Type::Encoder);

		auto allDevices = keyboards;
		allDevices.insert(allDevices.end(), encoders.begin(), encoders.end());

		for (const auto& dev: allDevices) {
			int id = dev->subscribeKeyEvents([this](const flx::hal::input::IInputDevice::KeyEvent& ev) {
				// Post to GUI thread
				if (!m_keyEventLabel || !m_keyCodeLabel) {
					return;
				}
				// Update labels — must be done from GUI task context
				// Use postToUi or rely on the fact we're already on GUI thread in LVGL timer
				onKeyEvent(ev);
			});
			if (id >= 0) {
				m_keySubscriptions.push_back({dev, id});
			}
		}
	}

	void unsubscribeKeyEvents() {
		for (auto& [dev, id]: m_keySubscriptions) {
			if (dev) {
				dev->unsubscribeKeyEvents(id);
			}
		}
		m_keySubscriptions.clear();
	}

	void onKeyEvent(const flx::hal::input::IInputDevice::KeyEvent& ev) {
		if (!m_keyEventLabel || !m_keyCodeLabel) {
			return;
		}
		lv_label_set_text(m_keyEventLabel, ev.pressed ? "Pressed" : "Released");
		char buf[32];
		snprintf(buf, sizeof(buf), "0x%04X (%c)",
			(unsigned)ev.keyCode,
			(ev.keyCode >= 32 && ev.keyCode < 127) ? (char)ev.keyCode : '?');
		lv_label_set_text(m_keyCodeLabel, buf);
	}

	lv_obj_t* m_keyEventLabel = nullptr;
	lv_obj_t* m_keyCodeLabel = nullptr;
	lv_obj_t* m_devicesSection = nullptr;

	struct KeySub {
		std::shared_ptr<flx::hal::input::IInputDevice> dev;
		int id;
	};
	std::vector<KeySub> m_keySubscriptions;
};

} // namespace System::Apps::Settings
