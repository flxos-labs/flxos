#include "settings/plugins/SettingsPluginRegistration.hpp"
#include "settings/bluetooth/BluetoothSettings.hpp"
#include "settings/customisation/CustomisationSettings.hpp"
#include "settings/developer/DeveloperSettings.hpp"
#include "settings/display/DisplaySettings.hpp"
#include "settings/gps/GpsSettings.hpp"
#include "settings/hotspot/HotspotSettings.hpp"
#include "settings/input/InputSettings.hpp"
#include "settings/locale/LocaleSettings.hpp"
#include "settings/plugins/SettingsPagePluginAdapter.hpp"
#include "settings/plugins/SettingsPluginRegistry.hpp"
#include "settings/power/PowerSettings.hpp"
#include "settings/storage/StorageSettings.hpp"
#include "settings/time/TimeSettings.hpp"
#include "settings/usb/UsbSettings.hpp"
#include "settings/webserver/WebServerSettings.hpp"
#include "settings/wifi/WiFiSettings.hpp"
#include <memory>

namespace System::Apps::Settings {

void registerBuiltInSettingsPlugins() {
	auto& registry = SettingsPluginRegistry::getInstance();

	registry.registerPlugin(
		{
			.id = "wifi",
			.displayName = "Wi-Fi",
			.icon = LV_SYMBOL_WIFI,
			.category = SettingsPluginCategory::Connectivity,
			.sortPriority = 10,
			.requiredCapabilities = flx::apps::AppCapability::WiFi,
		},
		[]() -> std::unique_ptr<ISettingsPlugin> {
			return std::make_unique<SettingsPagePluginAdapter<WiFiSettings>>();
		});

	registry.registerPlugin(
		{
			.id = "hotspot",
			.displayName = "Hotspot",
			.icon = LV_SYMBOL_WIFI,
			.category = SettingsPluginCategory::Connectivity,
			.sortPriority = 20,
			.requiredCapabilities = flx::apps::AppCapability::WiFi,
		},
		[]() -> std::unique_ptr<ISettingsPlugin> {
			return std::make_unique<SettingsPagePluginAdapter<HotspotSettings>>();
		});

	registry.registerPlugin(
		{
			.id = "bluetooth",
			.displayName = "Bluetooth",
			.icon = LV_SYMBOL_BLUETOOTH,
			.category = SettingsPluginCategory::Connectivity,
			.sortPriority = 30,
			.requiredCapabilities = flx::apps::AppCapability::Bluetooth,
		},
		[]() -> std::unique_ptr<ISettingsPlugin> {
			return std::make_unique<SettingsPagePluginAdapter<BluetoothSettings>>();
		});

	registry.registerPlugin(
		{
			.id = "display",
			.displayName = "Display",
			.icon = LV_SYMBOL_IMAGE,
			.category = SettingsPluginCategory::System,
			.sortPriority = 10,
			.requiredCapabilities = flx::apps::AppCapability::None,
		},
		[]() -> std::unique_ptr<ISettingsPlugin> {
			return std::make_unique<SettingsPagePluginAdapter<DisplaySettings>>();
		});

	registry.registerPlugin(
		{
			.id = "customisation",
			.displayName = "Customisation",
			.icon = LV_SYMBOL_EDIT,
			.category = SettingsPluginCategory::System,
			.sortPriority = 20,
			.requiredCapabilities = flx::apps::AppCapability::None,
		},
		[]() -> std::unique_ptr<ISettingsPlugin> {
			return std::make_unique<SettingsPagePluginAdapter<CustomisationSettings>>();
		});

	registry.registerPlugin(
		{
			.id = "power",
			.displayName = "Battery & Power",
			.icon = LV_SYMBOL_BATTERY_FULL,
			.category = SettingsPluginCategory::System,
			.sortPriority = 30,
			.requiredCapabilities = flx::apps::AppCapability::None,
		},
		[]() -> std::unique_ptr<ISettingsPlugin> {
			return std::make_unique<SettingsPagePluginAdapter<PowerSettings>>();
		});

	registry.registerPlugin(
		{
			.id = "time",
			.displayName = "Date & Time",
			.icon = LV_SYMBOL_REFRESH,
			.category = SettingsPluginCategory::System,
			.sortPriority = 40,
			.requiredCapabilities = flx::apps::AppCapability::None,
		},
		[]() -> std::unique_ptr<ISettingsPlugin> {
			return std::make_unique<SettingsPagePluginAdapter<TimeSettings>>();
		});

	registry.registerPlugin(
		{
			.id = "storage",
			.displayName = "Storage",
			.icon = LV_SYMBOL_DRIVE,
			.category = SettingsPluginCategory::System,
			.sortPriority = 50,
			.requiredCapabilities = flx::apps::AppCapability::None,
		},
		[]() -> std::unique_ptr<ISettingsPlugin> {
			return std::make_unique<SettingsPagePluginAdapter<StorageSettings>>();
		});

	registry.registerPlugin(
		{
			.id = "locale",
			.displayName = "Language & Region",
			.icon = LV_SYMBOL_AUDIO,
			.category = SettingsPluginCategory::System,
			.sortPriority = 60,
			.requiredCapabilities = flx::apps::AppCapability::None,
		},
		[]() -> std::unique_ptr<ISettingsPlugin> {
			return std::make_unique<SettingsPagePluginAdapter<LocaleSettings>>();
		});

	registry.registerPlugin(
		{
			.id = "input",
			.displayName = "Input Devices",
			.icon = LV_SYMBOL_KEYBOARD,
			.category = SettingsPluginCategory::Input,
			.sortPriority = 10,
			.requiredCapabilities = flx::apps::AppCapability::None,
		},
		[]() -> std::unique_ptr<ISettingsPlugin> {
			return std::make_unique<SettingsPagePluginAdapter<InputSettings>>();
		});

	registry.registerPlugin(
		{
			.id = "developer",
			.displayName = "Developer Options",
			.icon = LV_SYMBOL_LIST,
			.category = SettingsPluginCategory::Developer,
			.sortPriority = 10,
			.requiredCapabilities = flx::apps::AppCapability::None,
		},
		[]() -> std::unique_ptr<ISettingsPlugin> {
			return std::make_unique<SettingsPagePluginAdapter<DeveloperSettings>>();
		});

	registry.registerPlugin(
		{
			.id = "gps",
			.displayName = "GPS / GNSS",
			.icon = LV_SYMBOL_GPS,
			.category = SettingsPluginCategory::Hardware,
			.sortPriority = 10,
			.requiredCapabilities = flx::apps::AppCapability::None,
		},
		[]() -> std::unique_ptr<ISettingsPlugin> {
			return std::make_unique<SettingsPagePluginAdapter<GpsSettings>>();
		});

	registry.registerPlugin(
		{
			.id = "usb",
			.displayName = "USB",
			.icon = LV_SYMBOL_USB,
			.category = SettingsPluginCategory::Hardware,
			.sortPriority = 20,
			.requiredCapabilities = flx::apps::AppCapability::None,
		},
		[]() -> std::unique_ptr<ISettingsPlugin> {
			return std::make_unique<SettingsPagePluginAdapter<UsbSettings>>();
		});

	registry.registerPlugin(
		{
			.id = "webserver",
			.displayName = "Web Server",
			.icon = LV_SYMBOL_SETTINGS,
			.category = SettingsPluginCategory::Hardware,
			.sortPriority = 30,
			.requiredCapabilities = flx::apps::AppCapability::None,
		},
		[]() -> std::unique_ptr<ISettingsPlugin> {
			return std::make_unique<SettingsPagePluginAdapter<WebServerSettings>>();
		});
}

} // namespace System::Apps::Settings
