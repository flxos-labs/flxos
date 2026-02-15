#include <flx/system/managers/SettingsManager.hpp>
#include "cJSON.h"
#include "esp_timer.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <flx/core/Logger.hpp>
#include <flx/core/Observable.hpp>
#include <sys/stat.h>
#include <unistd.h>

static constexpr const char* TAG = "SettingsManager";
static constexpr const char* SETTINGS_PATH = "/system/settings.json";
static constexpr const char* SETTINGS_TMP_PATH = "/system/settings.tmp";

namespace flx::system {

const flx::services::ServiceManifest SettingsManager::serviceManifest = {
	.serviceId = "com.flxos.settings",
	.serviceName = "Settings",
	.dependencies = {},
	.priority = 10,
	.required = true,
	.autoStart = true,
	.guiRequired = false,
	.capabilities = flx::services::ServiceCapability::None,
	.description = "Persistent key-value settings storage",
};

bool SettingsManager::onStart() {
	if (isRunning()) return true;

	esp_timer_create_args_t const timer_args = {
		.callback = [](void* /*arg*/) {
			SettingsManager::getInstance().saveSettings();
		},
		.arg = nullptr,
		.dispatch_method = ESP_TIMER_TASK,
		.name = "settings_save",
		.skip_unhandled_events = true,
	};
	esp_timer_create(&timer_args, &m_save_timer);

	loadSettings();
	Log::info(TAG, "Settings service started");
	return true;
}

void SettingsManager::onStop() {
	// Save any pending settings before stopping
	if (m_save_timer) {
		esp_timer_stop(m_save_timer);
		saveSettings();
		esp_timer_delete(m_save_timer);
		m_save_timer = nullptr;
	}
	if (m_json_cache) {
		cJSON_Delete((cJSON*)m_json_cache);
		m_json_cache = nullptr;
	}
	Log::info(TAG, "Settings service stopped");
}

void SettingsManager::registerSetting(const std::string& key, flx::Observable<int32_t>& observable) {
	m_registeredSettings[key] = {Setting::Type::INT, &observable};

	// Subscribe to changes to trigger save
	observable.subscribe([this](const int32_t&) {
		this->triggerSave();
	});

	// If we have cached JSON, apply the value now
	if (m_json_cache) {
		cJSON* item = cJSON_GetObjectItem((cJSON*)m_json_cache, key.c_str());
		if (item && cJSON_IsNumber(item)) {
			observable.set(item->valueint);
		}
	}
}

void SettingsManager::registerSetting(const std::string& key, flx::StringObservable& observable) {
	m_registeredSettings[key] = {Setting::Type::STRING, &observable};

	// Subscribe to changes to trigger save
	observable.subscribe([this](const std::string&) {
		this->triggerSave();
	});

	// If we have cached JSON, apply the value now
	if (m_json_cache) {
		cJSON* item = cJSON_GetObjectItem((cJSON*)m_json_cache, key.c_str());
		if (item && cJSON_IsString(item)) {
			observable.set(item->valuestring);
		}
	}
}

void SettingsManager::triggerSave() {
	if (m_save_timer) {
		esp_timer_stop(m_save_timer);
		esp_timer_start_once(m_save_timer, 2000000); // 2 seconds
	}
}

void SettingsManager::loadSettings() {
	FILE* f = fopen(SETTINGS_PATH, "r");
	if (!f) {
		Log::info(TAG, "No settings file found, using defaults");
		return;
	}

	fseek(f, 0, SEEK_END);
	long len = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (len > 0) {
		char* buf = (char*)malloc(len + 1);
		if (buf) {
			if (fread(buf, 1, len, f) == (size_t)len) {
				buf[len] = 0;
				if (m_json_cache) cJSON_Delete((cJSON*)m_json_cache);
				m_json_cache = cJSON_Parse(buf);
			}
			free(buf);
		}
	}
	fclose(f);
	Log::info(TAG, "Settings loaded and cached");
}

void SettingsManager::saveSettings() {
	cJSON* json = cJSON_CreateObject();

	for (auto const& [key, setting]: m_registeredSettings) {
		if (setting.type == Setting::Type::INT) {
			auto* obs = (flx::Observable<int32_t>*)setting.observable;
			cJSON_AddNumberToObject(json, key.c_str(), obs->get());
		} else {
			auto* obs = (flx::StringObservable*)setting.observable;
			std::string val = obs->get();
			cJSON_AddStringToObject(json, key.c_str(), val.c_str());
		}
	}

	char* str = cJSON_Print(json);
	if (str) {
		FILE* f = fopen(SETTINGS_TMP_PATH, "w");
		if (f) {
			fprintf(f, "%s", str);
			fsync(fileno(f));
			fclose(f);
			unlink(SETTINGS_PATH);
			rename(SETTINGS_TMP_PATH, SETTINGS_PATH);
			Log::info(TAG, "Settings saved successfully");

			// Also update cache
			if (m_json_cache) {
				cJSON_Delete((cJSON*)m_json_cache);
			}
			m_json_cache = cJSON_Parse(str);
		} else {
			Log::error(TAG, "Failed to open settings file for writing");
		}
		free(str);
	}
	cJSON_Delete(json);
}

} // namespace System
