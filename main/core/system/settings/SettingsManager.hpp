#pragma once

#include "core/common/Observable.hpp"
#include <functional>
#include <map>
#include <memory>
#include <string>

#include "esp_timer.h"

namespace System {

class SettingsManager {
public:

	static SettingsManager& getInstance();

	void init();

	// Registration
	void registerSetting(const std::string& key, Observable<int32_t>& observable);
	void registerSetting(const std::string& key, StringObservable& observable);

	void triggerSave();
	void saveSettings();
	void loadSettings();

private:

	SettingsManager() = default;
	~SettingsManager() = default;
	SettingsManager(const SettingsManager&) = delete;
	SettingsManager& operator=(const SettingsManager&) = delete;

	struct Setting {
		enum class Type { INT,
						  STRING } type;
		void* observable;
	};

	std::map<std::string, Setting> m_registeredSettings {};
	void* m_json_cache = nullptr; // cJSON*

	esp_timer_handle_t m_save_timer = nullptr;

	bool m_is_init = false;
};

} // namespace System
