#pragma once

#include "lvgl.h"
#include <cstdint>
#include <flx/apps/AppManifest.hpp>
#include <functional>
#include <memory>
#include <string>

namespace System::Apps::Settings {

enum class SettingsPluginCategory : uint8_t {
	Connectivity = 0,
	System = 1,
	Input = 2,
	Developer = 3,
	Hardware = 4,
};

struct SettingsPluginManifest {
	std::string id;
	std::string displayName;
	const char* icon = nullptr;
	SettingsPluginCategory category = SettingsPluginCategory::System;
	int sortPriority = 100;
	flx::apps::AppCapability requiredCapabilities = flx::apps::AppCapability::None;
};

struct SettingsPluginDescriptor {
	SettingsPluginManifest manifest;
	uint64_t generation = 0;
};

class ISettingsPlugin {
public:

	virtual ~ISettingsPlugin() = default;

	virtual void onAttach(lv_obj_t* parent, std::function<void()> onBack) = 0;
	virtual void onDetach() = 0;
	virtual void onShow() = 0;
	virtual void onHide() = 0;
	virtual void onSave() = 0;
};

using SettingsPluginFactory = std::function<std::unique_ptr<ISettingsPlugin>()>;

} // namespace System::Apps::Settings
