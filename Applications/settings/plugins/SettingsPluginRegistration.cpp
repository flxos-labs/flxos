#include "settings/plugins/SettingsPluginRegistration.hpp"
#include "settings/bluetooth/BluetoothSettings.hpp"
#include "settings/customisation/CustomisationSettings.hpp"
#include "settings/display/DisplaySettings.hpp"
#include "settings/hotspot/HotspotSettings.hpp"
#include "settings/plugins/SettingsPagePluginAdapter.hpp"
#include "settings/plugins/SettingsPluginRegistry.hpp"
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
}

} // namespace System::Apps::Settings
