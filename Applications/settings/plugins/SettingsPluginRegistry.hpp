#pragma once

#include "settings/plugins/ISettingsPlugin.hpp"
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace System::Apps::Settings {

class SettingsPluginRegistry {
public:

	using ObserverToken = uint32_t;
	using ChangeCallback = std::function<void()>;

	static SettingsPluginRegistry& getInstance();

	bool registerPlugin(const SettingsPluginManifest& manifest, SettingsPluginFactory factory);
	bool unregisterPlugin(const std::string& id);

	std::vector<SettingsPluginDescriptor> getAllPlugins() const;
	std::vector<SettingsPluginDescriptor> getAvailablePlugins() const;
	std::unique_ptr<ISettingsPlugin> createPlugin(const std::string& id) const;

	bool hasPlugin(const std::string& id) const;

	ObserverToken addObserver(ChangeCallback callback);
	bool removeObserver(ObserverToken token);

private:

	struct Entry {
		SettingsPluginDescriptor descriptor;
		SettingsPluginFactory factory;
	};

	static bool isSupported(const SettingsPluginManifest& manifest);
	static void sortDescriptors(std::vector<SettingsPluginDescriptor>& descriptors);
	static void notifyObservers(const std::vector<ChangeCallback>& callbacks);

	mutable std::mutex m_mutex;
	std::unordered_map<std::string, Entry> m_entries;
	std::unordered_map<ObserverToken, ChangeCallback> m_observers;
	uint64_t m_nextGeneration = 1;
	ObserverToken m_nextObserverToken = 1;
};

} // namespace System::Apps::Settings
