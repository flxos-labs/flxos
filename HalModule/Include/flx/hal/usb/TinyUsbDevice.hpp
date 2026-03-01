#pragma once

#include <flx/hal/DeviceBase.hpp>
#include <flx/hal/usb/IUsbDevice.hpp>

namespace flx::hal::usb {

/**
 * @brief Concrete implementation of USB device using TinyUSB via ESP-IDF.
 */
class TinyUsbDevice : public DeviceBase<IUsbDevice> {
public:

	TinyUsbDevice();
	~TinyUsbDevice() override;

	// ── IDevice ───────────────────────────────────────────────────────────
	std::string_view getName() const override { return "ESP TinyUSB"; }
	std::string_view getDescription() const override { return "TinyUSB device driver (MSC/CDC)"; }
	bool start() override;
	bool stop() override;
	Type getType() const override { return IUsbDevice::getType(); }

	// ── IUsbDevice ────────────────────────────────────────────────────────
	bool isSupported() const override;
	UsbMode getCurrentMode() const override;

	bool startMassStorage(UsbMode mode) override;
	bool startCdcSerial() override;
	void stopMode() override;

	bool canReboot(UsbMode mode) const override;
	void rebootInto(UsbMode mode) override;

private:

	UsbMode m_currentMode = UsbMode::None;
};

} // namespace flx::hal::usb
