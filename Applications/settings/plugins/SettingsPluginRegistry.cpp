#include "SettingsPluginRegistry.hpp"
#include <algorithm>
#include <cstdint>
#include <flx/core/Logger.hpp>
#include <flx/hal/HardwareCapabilities.hpp>

static constexpr const char* TAG = "SettingsPluginRegistry";

namespace System::Apps::Settings {

SettingsPluginRegistry& SettingsPluginRegistry::getInstance() {
	static SettingsPluginRegistry instance;
	return instance;
}

bool SettingsPluginRegistry::registerPlugin(const SettingsPluginManifest& manifest, SettingsPluginFactory factory) {
	if (manifest.id.empty() || manifest.displayName.empty()) {
		Log::warn(TAG, "Ignoring settings plugin with incomplete manifest");
		return false;
	}

	if (!factory) {
		Log::warn(TAG, "Ignoring settings plugin '%s' without a factory", manifest.id.c_str());
		return false;
	}

	std::vector<ChangeCallback> callbacks;
	bool existed = false;

	{
		std::lock_guard<std::mutex> lock(m_mutex);
		auto& entry = m_entries[manifest.id];
		existed = !entry.descriptor.manifest.id.empty();
		entry.descriptor.manifest = manifest;
		entry.descriptor.generation = m_nextGeneration++;
		entry.factory = std::move(factory);

		callbacks.reserve(m_observers.size());
		for (const auto& [token, callback]: m_observers) {
			(void)token;
			if (callback) {
				callbacks.push_back(callback);
			}
		}
	}

	Log::info(
		TAG, "%s settings plugin: %s (%s)",
		existed ? "Updated" : "Registered",
		manifest.displayName.c_str(), manifest.id.c_str());

	notifyObservers(callbacks);
	return true;
}

bool SettingsPluginRegistry::unregisterPlugin(const std::string& id) {
	std::vector<ChangeCallback> callbacks;
	bool removed = false;

	{
		std::lock_guard<std::mutex> lock(m_mutex);
		removed = m_entries.erase(id) > 0;
		if (!removed) {
			return false;
		}

		callbacks.reserve(m_observers.size());
		for (const auto& [token, callback]: m_observers) {
			(void)token;
			if (callback) {
				callbacks.push_back(callback);
			}
		}
	}

	Log::info(TAG, "Unregistered settings plugin: %s", id.c_str());
	notifyObservers(callbacks);
	return true;
}

std::vector<SettingsPluginDescriptor> SettingsPluginRegistry::getAllPlugins() const {
	std::lock_guard<std::mutex> lock(m_mutex);

	std::vector<SettingsPluginDescriptor> descriptors;
	descriptors.reserve(m_entries.size());
	for (const auto& [id, entry]: m_entries) {
		(void)id;
		descriptors.push_back(entry.descriptor);
	}

	sortDescriptors(descriptors);
	return descriptors;
}

std::vector<SettingsPluginDescriptor> SettingsPluginRegistry::getAvailablePlugins() const {
	std::lock_guard<std::mutex> lock(m_mutex);

	std::vector<SettingsPluginDescriptor> descriptors;
	descriptors.reserve(m_entries.size());
	for (const auto& [id, entry]: m_entries) {
		(void)id;
		if (isSupported(entry.descriptor.manifest)) {
			descriptors.push_back(entry.descriptor);
		}
	}

	sortDescriptors(descriptors);
	return descriptors;
}

std::unique_ptr<ISettingsPlugin> SettingsPluginRegistry::createPlugin(const std::string& id) const {
	SettingsPluginFactory factory;

	{
		std::lock_guard<std::mutex> lock(m_mutex);
		auto it = m_entries.find(id);
		if (it == m_entries.end()) {
			return nullptr;
		}
		factory = it->second.factory;
	}

	return factory ? factory() : nullptr;
}

bool SettingsPluginRegistry::hasPlugin(const std::string& id) const {
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_entries.count(id) > 0;
}

SettingsPluginRegistry::ObserverToken SettingsPluginRegistry::addObserver(ChangeCallback callback) {
	if (!callback) {
		return 0;
	}

	std::lock_guard<std::mutex> lock(m_mutex);
	ObserverToken const token = m_nextObserverToken++;
	m_observers[token] = std::move(callback);
	return token;
}

bool SettingsPluginRegistry::removeObserver(ObserverToken token) {
	if (token == 0) {
		return false;
	}

	std::lock_guard<std::mutex> lock(m_mutex);
	return m_observers.erase(token) > 0;
}

bool SettingsPluginRegistry::isSupported(const SettingsPluginManifest& manifest) {
	using flx::apps::AppCapability;
	using flx::apps::hasCapability;

	if (manifest.requiredCapabilities == AppCapability::None) {
		return true;
	}

	auto const caps = flx::hal::getCapabilities();
	if (hasCapability(manifest.requiredCapabilities, AppCapability::WiFi) && !caps.hasWifi()) {
		return false;
	}
	if (hasCapability(manifest.requiredCapabilities, AppCapability::Bluetooth) && !caps.hasBluetooth()) {
		return false;
	}
	if (hasCapability(manifest.requiredCapabilities, AppCapability::Storage) && !caps.hasStorage()) {
		return false;
	}
	if (hasCapability(manifest.requiredCapabilities, AppCapability::Camera) && !caps.hasCamera()) {
		return false;
	}
	if (hasCapability(manifest.requiredCapabilities, AppCapability::GPIO) && !caps.hasGpio()) {
		return false;
	}
	if (hasCapability(manifest.requiredCapabilities, AppCapability::I2C) && !caps.hasI2C()) {
		return false;
	}
	if (hasCapability(manifest.requiredCapabilities, AppCapability::SPI) && !caps.hasSpi()) {
		return false;
	}
	if (hasCapability(manifest.requiredCapabilities, AppCapability::UART) && !caps.hasUart()) {
		return false;
	}

	return true;
}

void SettingsPluginRegistry::sortDescriptors(std::vector<SettingsPluginDescriptor>& descriptors) {
	std::sort(
		descriptors.begin(), descriptors.end(),
		[](const SettingsPluginDescriptor& lhs, const SettingsPluginDescriptor& rhs) {
			auto const lhsCategory = static_cast<uint8_t>(lhs.manifest.category);
			auto const rhsCategory = static_cast<uint8_t>(rhs.manifest.category);
			if (lhsCategory != rhsCategory) {
				return lhsCategory < rhsCategory;
			}
			if (lhs.manifest.sortPriority != rhs.manifest.sortPriority) {
				return lhs.manifest.sortPriority < rhs.manifest.sortPriority;
			}
			return lhs.manifest.displayName < rhs.manifest.displayName;
		});
}

void SettingsPluginRegistry::notifyObservers(const std::vector<ChangeCallback>& callbacks) {
	for (const auto& callback: callbacks) {
		if (callback) {
			callback();
		}
	}
}

} // namespace System::Apps::Settings
