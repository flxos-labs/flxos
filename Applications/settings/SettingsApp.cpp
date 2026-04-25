#include "SettingsApp.hpp"
#include <array>
#include <flx/apps/AppManifest.hpp>
#include <utility>

using namespace flx::apps;
using namespace flx::ui::common;

namespace System::Apps {

const AppManifest SettingsApp::manifest = {
	.appId = "com.flxos.settings",
	.appName = "Settings",
	.appIcon = LV_SYMBOL_SETTINGS,
	.appVersionName = "0.1.0",
	.appVersionCode = 1,
	.category = AppCategory::System,
	.flags = AppFlags::SingleInstance,
	.location = AppLocation::internal(),
	.description = "System configuration and preferences",
	.sortPriority = 10,
	.capabilities = AppCapability::None,
	.requiredServices = {},
	.supportedMimeTypes = {},
	.urlSchemes = {},
	.createApp = []() -> std::shared_ptr<App> { return std::make_shared<SettingsApp>(); }};

void SettingsApp::createUI(void* parent) {
	m_container = static_cast<lv_obj_t*>(parent);
	m_mainList = nullptr;
	m_activePluginId.clear();
	m_plugins.clear();
	m_pluginOrder.clear();
	m_buttonBindings.clear();
	m_pluginRefreshPending.store(false);

	auto& registry = Settings::SettingsPluginRegistry::getInstance();
	if (m_registryObserver == 0) {
		m_registryObserver = registry.addObserver([this]() {
			m_pluginRefreshPending.store(true);
		});
	}

	syncPluginsFromRegistry();
	showMainSettings();
}

void SettingsApp::onStop() {
	if (m_registryObserver != 0) {
		Settings::SettingsPluginRegistry::getInstance().removeObserver(m_registryObserver);
		m_registryObserver = 0;
	}

	for (auto& [pluginId, plugin]: m_plugins) {
		if (!plugin.instance) {
			continue;
		}

		if (pluginId == m_activePluginId) {
			plugin.instance->onSave();
			plugin.instance->onHide();
		}

		plugin.instance->onDetach();
	}

	m_container = nullptr;
	m_mainList = nullptr;
	m_activePluginId.clear();
	m_plugins.clear();
	m_pluginOrder.clear();
	m_buttonBindings.clear();
	m_pluginRefreshPending.store(false);
}

void SettingsApp::onPause() {
	if (!m_activePluginId.empty()) {
		deactivatePlugin(m_activePluginId);
	}
	m_pluginRefreshPending.store(false);
}

void SettingsApp::update() {
	if (!m_pluginRefreshPending.exchange(false) || m_container == nullptr) {
		return;
	}

	syncPluginsFromRegistry();
}

void SettingsApp::showMainSettings() {
	syncPluginsFromRegistry();

	if (!m_activePluginId.empty()) {
		deactivatePlugin(m_activePluginId);
	}

	rebuildMainList();
	if (m_mainList) {
		lv_obj_remove_flag(m_mainList, LV_OBJ_FLAG_HIDDEN);
	}
}

void SettingsApp::showPlugin(const std::string& pluginId) {
	syncPluginsFromRegistry();

	auto it = m_plugins.find(pluginId);
	if (it == m_plugins.end() || !it->second.instance) {
		showMainSettings();
		return;
	}

	if (!m_activePluginId.empty() && m_activePluginId != pluginId) {
		deactivatePlugin(m_activePluginId);
	}

	if (m_mainList) {
		lv_obj_add_flag(m_mainList, LV_OBJ_FLAG_HIDDEN);
	}

	it->second.instance->onShow();
	m_activePluginId = pluginId;
}

void SettingsApp::syncPluginsFromRegistry() {
	if (m_container == nullptr) {
		return;
	}

	auto& registry = Settings::SettingsPluginRegistry::getInstance();
	auto const descriptors = registry.getAvailablePlugins();

	std::unordered_map<std::string, Settings::SettingsPluginDescriptor> desired;
	desired.reserve(descriptors.size());
	for (const auto& descriptor: descriptors) {
		desired.emplace(descriptor.manifest.id, descriptor);
	}

	bool activePluginRemoved = false;
	bool pluginSetChanged = false;

	for (auto it = m_plugins.begin(); it != m_plugins.end();) {
		auto const desiredIt = desired.find(it->first);
		if (desiredIt != desired.end()) {
			++it;
			continue;
		}

		if (it->second.instance) {
			if (m_activePluginId == it->first) {
				it->second.instance->onSave();
				it->second.instance->onHide();
				m_activePluginId.clear();
				activePluginRemoved = true;
			}

			it->second.instance->onDetach();
		}

		pluginSetChanged = true;
		it = m_plugins.erase(it);
	}

	for (const auto& descriptor: descriptors) {
		bool wasActive = false;

		auto existing = m_plugins.find(descriptor.manifest.id);
		if (existing != m_plugins.end() && existing->second.descriptor.generation == descriptor.generation) {
			existing->second.descriptor = descriptor;
			continue;
		}

		if (existing != m_plugins.end()) {
			wasActive = (m_activePluginId == descriptor.manifest.id);
			if (existing->second.instance) {
				if (wasActive) {
					existing->second.instance->onSave();
					existing->second.instance->onHide();
					m_activePluginId.clear();
				}

				existing->second.instance->onDetach();
			}

			m_plugins.erase(existing);
			pluginSetChanged = true;
		}

		auto instance = registry.createPlugin(descriptor.manifest.id);
		if (!instance) {
			continue;
		}

		instance->onAttach(m_container, [this]() { showMainSettings(); });
		m_plugins[descriptor.manifest.id] = RuntimePlugin {
			.descriptor = descriptor,
			.instance = std::move(instance),
		};
		pluginSetChanged = true;

		if (wasActive) {
			auto active = m_plugins.find(descriptor.manifest.id);
			if (active != m_plugins.end() && active->second.instance) {
				active->second.instance->onShow();
				m_activePluginId = descriptor.manifest.id;
			}
		}
	}

	m_pluginOrder.clear();
	m_pluginOrder.reserve(descriptors.size());
	for (const auto& descriptor: descriptors) {
		m_pluginOrder.push_back(descriptor.manifest.id);
	}

	if (m_mainList && pluginSetChanged) {
		rebuildMainList();
		if (activePluginRemoved) {
			lv_obj_remove_flag(m_mainList, LV_OBJ_FLAG_HIDDEN);
		}
	}
}

void SettingsApp::rebuildMainList() {
	if (m_container == nullptr) {
		return;
	}

	if (m_mainList == nullptr) {
		m_mainList = lv_list_create(m_container);
		lv_obj_set_size(m_mainList, lv_pct(100), lv_pct(100));
		lv_obj_set_style_border_width(m_mainList, 0, 0);
	} else {
		lv_obj_clean(m_mainList);
	}

	m_buttonBindings.clear();

	static constexpr std::array<Settings::SettingsPluginCategory, 5> kCategoryOrder = {
		Settings::SettingsPluginCategory::Connectivity,
		Settings::SettingsPluginCategory::System,
		Settings::SettingsPluginCategory::Input,
		Settings::SettingsPluginCategory::Developer,
		Settings::SettingsPluginCategory::Hardware,
	};

	bool hasPlugins = false;

	for (auto const category: kCategoryOrder) {
		bool addedCategoryHeader = false;

		for (const auto& pluginId: m_pluginOrder) {
			auto const it = m_plugins.find(pluginId);
			if (it == m_plugins.end()) {
				continue;
			}

			auto const& manifest = it->second.descriptor.manifest;
			if (manifest.category != category) {
				continue;
			}

			if (!addedCategoryHeader) {
				lv_list_add_text(m_mainList, categoryTitle(category));
				addedCategoryHeader = true;
			}

			auto* button = add_list_btn(
				m_mainList,
				manifest.icon ? manifest.icon : LV_SYMBOL_SETTINGS,
				manifest.displayName.c_str());

			auto binding = std::make_unique<ButtonBinding>();
			binding->app = this;
			binding->pluginId = manifest.id;

			lv_obj_add_event_cb(
				button,
				[](lv_event_t* e) {
					auto* binding = static_cast<ButtonBinding*>(lv_event_get_user_data(e));
					if (binding && binding->app) {
						binding->app->showPlugin(binding->pluginId);
					}
				},
				LV_EVENT_CLICKED, binding.get());

			m_buttonBindings.push_back(std::move(binding));
			hasPlugins = true;
		}
	}

	if (!hasPlugins) {
		lv_list_add_text(m_mainList, "No settings available");
	}
}

void SettingsApp::deactivatePlugin(const std::string& pluginId, bool saveState) {
	auto const it = m_plugins.find(pluginId);
	if (it == m_plugins.end() || !it->second.instance) {
		if (m_activePluginId == pluginId) {
			m_activePluginId.clear();
		}
		return;
	}

	if (saveState) {
		it->second.instance->onSave();
	}

	it->second.instance->onHide();
	if (m_activePluginId == pluginId) {
		m_activePluginId.clear();
	}
}

const char* SettingsApp::categoryTitle(Settings::SettingsPluginCategory category) {
	switch (category) {
		case Settings::SettingsPluginCategory::Connectivity:
			return "Connectivity";
		case Settings::SettingsPluginCategory::System:
			return "System";
		case Settings::SettingsPluginCategory::Input:
			return "Input";
		case Settings::SettingsPluginCategory::Developer:
			return "Developer";
		case Settings::SettingsPluginCategory::Hardware:
			return "Hardware";
	}

	return "Other";
}

} // namespace System::Apps
