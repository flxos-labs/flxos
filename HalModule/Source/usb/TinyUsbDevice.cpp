#include <flx/hal/usb/TinyUsbDevice.hpp>
#include <flx/core/Logger.hpp>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// If TinyUSB is enabled in sdkconfig, we'd include its headers here.
// #include "tinyusb.h"
// #include "tusb_msc_storage.h"

namespace flx::hal::usb {

static constexpr std::string_view TAG = "TinyUsbDevice";

TinyUsbDevice::TinyUsbDevice() {
	this->setState(State::Uninitialized);
}

TinyUsbDevice::~TinyUsbDevice() {
	if (getState() == State::Ready) {
		stop();
	}
}

bool TinyUsbDevice::start() {
	this->setState(State::Starting);

#if CONFIG_TINYUSB_ENABLED || CONFIG_USB_DEVICE_ENABLED
	flx::Log::info(TAG, "USB Driver initialized in default mode");
	m_currentMode = UsbMode::Default;
	this->setState(State::Ready);
	return true;
#else
	flx::Log::warn(TAG, "TinyUSB support not enabled in ESP-IDF config");
	this->setState(State::Error);
	return false;
#endif
}

bool TinyUsbDevice::stop() {
	stopMode();
	this->setState(State::Stopped);
	return true;
}

bool TinyUsbDevice::isSupported() const {
#if CONFIG_TINYUSB_ENABLED || CONFIG_USB_DEVICE_ENABLED
	return true;
#else
	return false;
#endif
}

IUsbDevice::UsbMode TinyUsbDevice::getCurrentMode() const {
	return m_currentMode;
}

bool TinyUsbDevice::startMassStorage(UsbMode mode) {
	if (!isSupported()) return false;
	if (m_currentMode == mode) return true;

	stopMode();
	
	flx::Log::info(TAG, "Starting Mass Storage Mode...");
	// Insert actual tinyusb MSC init code here.
	// E.g., tinyusb_config_t tusb_cfg = { ... };
	// tinyusb_driver_install(&tusb_cfg);

	m_currentMode = mode;
	return true;
}

bool TinyUsbDevice::startCdcSerial() {
	if (!isSupported()) return false;
	if (m_currentMode == UsbMode::CdcSerial) return true;

	stopMode();
	
	flx::Log::info(TAG, "Starting CDC Serial Mode...");
	// Insert actual tinyusb CDC init code here.

	m_currentMode = UsbMode::CdcSerial;
	return true;
}

void TinyUsbDevice::stopMode() {
	if (m_currentMode != UsbMode::None) {
		flx::Log::info(TAG, "Stopping USB driver...");
		// tinyusb_driver_uninstall(); // or equivalent teardown
		m_currentMode = UsbMode::None;
	}
}

bool TinyUsbDevice::canReboot(UsbMode mode) const {
	(void)mode;
	return true; // ESP32 series can always soft reboot
}

void TinyUsbDevice::rebootInto(UsbMode mode) {
	flx::Log::warn(TAG, "Rebooting into USB mode %d...", static_cast<int>(mode));
	// Can write mode to RTC memory here so next boot knows what to do
	vTaskDelay(pdMS_TO_TICKS(100));
	esp_restart();
}

} // namespace flx::hal::usb
