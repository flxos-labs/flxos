#include <Config.hpp>
#include <flx/hal/HardwareCapabilities.hpp>

#include <flx/hal/DeviceRegistry.hpp>
#include <flx/hal/IDevice.hpp>

namespace flx::hal {

bool HardwareCapabilities::hasDisplay() const { return flx::config::display.enabled; }
bool HardwareCapabilities::hasTouch() const { return flx::config::touch.enabled; }
bool HardwareCapabilities::hasSdCard() const { return flx::config::sdcard.enabled; }
bool HardwareCapabilities::hasBattery() const { return flx::config::battery.enabled; }
bool HardwareCapabilities::hasKeyboard() const { return flx::config::capabilities.keyboard; }
bool HardwareCapabilities::hasGps() const { return flx::config::capabilities.gps; }
bool HardwareCapabilities::hasUsb() const { return flx::config::usb.tinyUsb; }
bool HardwareCapabilities::hasWifi() const { return flx::config::capabilities.wifi; }
bool HardwareCapabilities::hasBluetooth() const { return flx::config::capabilities.bluetooth; }
bool HardwareCapabilities::hasI2C() const { return flx::hal::DeviceRegistry::getInstance().findFirst<flx::hal::IDevice>(flx::hal::IDevice::Type::I2c) != nullptr; }
bool HardwareCapabilities::hasSpi() const { return flx::hal::DeviceRegistry::getInstance().findFirst<flx::hal::IDevice>(flx::hal::IDevice::Type::Spi) != nullptr; }
bool HardwareCapabilities::hasUart() const { return flx::hal::DeviceRegistry::getInstance().findFirst<flx::hal::IDevice>(flx::hal::IDevice::Type::Uart) != nullptr; }
bool HardwareCapabilities::hasGpio() const { return flx::hal::DeviceRegistry::getInstance().findFirst<flx::hal::IDevice>(flx::hal::IDevice::Type::Gpio) != nullptr; }

uint16_t HardwareCapabilities::displayWidth() const { return flx::config::display.width; }
uint16_t HardwareCapabilities::displayHeight() const { return flx::config::display.height; }

const char* HardwareCapabilities::chipModel() const { return "Unknown"; } // Placeholder
uint32_t HardwareCapabilities::flashSizeBytes() const { return 0; } // Placeholder
uint32_t HardwareCapabilities::psramSizeBytes() const { return 0; } // Placeholder

HardwareCapabilities getActiveCapabilities() {
	return HardwareCapabilities(); // Placeholder, but gets capability flags
}

} // namespace flx::hal
