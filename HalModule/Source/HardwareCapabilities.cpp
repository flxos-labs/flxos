#include <flx/hal/HardwareCapabilities.hpp>

namespace flx::hal {

bool HardwareCapabilities::hasDisplay() const { return flx::config::hardware_display_enabled; }
bool HardwareCapabilities::hasTouch() const { return flx::config::hardware_touch_enabled; }
bool HardwareCapabilities::hasSdCard() const { return flx::config::hardware_sdcard_enabled; }
bool HardwareCapabilities::hasBattery() const { return flx::config::hardware_power_enabled; }
bool HardwareCapabilities::hasKeyboard() const { return flx::config::hardware_keyboard_enabled; }
bool HardwareCapabilities::hasGps() const { return flx::config::hardware_gps_enabled; }
bool HardwareCapabilities::hasUsb() const { return flx::config::hardware_usb_enabled; }
bool HardwareCapabilities::hasWifi() const { return flx::config::hardware_wifi_enabled; }
bool HardwareCapabilities::hasBluetooth() const { return flx::config::hardware_bluetooth_enabled; }
bool HardwareCapabilities::hasI2C() const { return flx::config::hardware_i2c_enabled; }
bool HardwareCapabilities::hasSpi() const { return flx::config::hardware_spi_enabled; }
bool HardwareCapabilities::hasUart() const { return flx::config::hardware_uart_enabled; }
bool HardwareCapabilities::hasGpio() const { return flx::config::hardware_gpio_enabled; }

uint16_t HardwareCapabilities::displayWidth() const { return flx::config::hardware_display_width; }
uint16_t HardwareCapabilities::displayHeight() const { return flx::config::hardware_display_height; }

const char* HardwareCapabilities::chipModel() const { return "Unknown"; } // Placeholder
uint32_t HardwareCapabilities::flashSizeBytes() const { return 0; } // Placeholder
uint32_t HardwareCapabilities::psramSizeBytes() const { return 0; } // Placeholder

HardwareCapabilities getActiveCapabilities() {
	return HardwareCapabilities(); // Placeholder, but gets capability flags
}

} // namespace flx::hal
