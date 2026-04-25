#pragma once

#include "lvgl.h"
#include "settings/plugins/SettingsPluginRegistry.hpp"
#include <atomic>
#include <flx/apps/App.hpp>
#include <flx/apps/AppManifest.hpp>
#include <flx/ui/common/SettingsCommon.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace System::Apps {

using flx::apps::AppManifest;
class SettingsApp : public flx::apps::App {
public:

	std::string getPackageName() const override { return "com.flxos.settings"; }
	std::string getAppName() const override { return "Settings"; }
	std::string getVersion() const override { return "1.2.0"; }
	const void* getIcon() const override { return LV_SYMBOL_SETTINGS; }

	static const AppManifest manifest;

	void createUI(void* parent) override;
	void onPause() override;
	void onStop() override;
	void update() override;

private:

	struct ButtonBinding {
		SettingsApp* app = nullptr;
		std::string pluginId;
	};

	struct RuntimePlugin {
		Settings::SettingsPluginDescriptor descriptor;
		std::unique_ptr<Settings::ISettingsPlugin> instance;
	};

	lv_obj_t* m_container = nullptr;
	lv_obj_t* m_mainList = nullptr;
	std::string m_activePluginId;
	std::unordered_map<std::string, RuntimePlugin> m_plugins;
	std::vector<std::string> m_pluginOrder;
	std::vector<std::unique_ptr<ButtonBinding>> m_buttonBindings;
	std::atomic_bool m_pluginRefreshPending = false;
	Settings::SettingsPluginRegistry::ObserverToken m_registryObserver = 0;

	void showMainSettings();
	void showPlugin(const std::string& pluginId);
	void syncPluginsFromRegistry();
	void rebuildMainList();
	void deactivatePlugin(const std::string& pluginId, bool saveState = true);

	static const char* categoryTitle(Settings::SettingsPluginCategory category);
};

} // namespace System::Apps
