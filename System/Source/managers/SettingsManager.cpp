#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <esp_timer.h>
#include <flx/core/EventBus.hpp>
#include <flx/core/Logger.hpp>
#include <flx/core/Observable.hpp>
#include <flx/core/Value.hpp>
#include <flx/system/managers/SettingsManager.hpp>
#include <sys/stat.h>
#include <unistd.h>

static constexpr const char* TAG = "SettingsManager";
static constexpr const char* SETTINGS_PATH = "/system/settings.json";
static constexpr const char* SETTINGS_TMP_PATH = "/system/settings.tmp";

namespace flx::system {

namespace {

void appendEscapedJsonString(const std::string& input, std::string& output) {
	static constexpr char kHexDigits[] = "0123456789abcdef";

	for (const unsigned char c: input) {
		switch (c) {
			case '\\':
				output += "\\\\";
				break;
			case '\"':
				output += "\\\"";
				break;
			case '\b':
				output += "\\b";
				break;
			case '\f':
				output += "\\f";
				break;
			case '\n':
				output += "\\n";
				break;
			case '\r':
				output += "\\r";
				break;
			case '\t':
				output += "\\t";
				break;
			default:
				if (c < 0x20) {
					output += "\\u00";
					output += kHexDigits[(c >> 4) & 0x0F];
					output += kHexDigits[c & 0x0F];
				} else {
					output += static_cast<char>(c);
				}
				break;
		}
	}
}

} // namespace

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

	registerSetting("locale.language_index", m_languageIndex);
	registerSetting("locale.time_format_24h", m_timeFormat24h);
	registerSetting("locale.date_format_index", m_dateFormatIndex);
	registerSetting("developer.verbose_logging", m_verboseLogging);
	registerSetting("developer.diagnostic_overlay", m_diagnosticOverlay);

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
	m_cachedSettings.reset();
	Log::info(TAG, "Settings service stopped");
}

void SettingsManager::registerSetting(const std::string& key, flx::Observable<int32_t>& observable) {
	m_registeredSettings[key] = {Setting::Type::INT, &observable};

	// Subscribe to changes to trigger save
	observable.subscribe([this](const int32_t&) {
		this->triggerSave();
	});

	// If we have cached settings, apply the value now
	if (m_cachedSettings) {
		const flx::core::FlxValueView valueNode = m_cachedSettings->root().child(key);
		if (valueNode.valid() && valueNode.isIntScalar()) {
			observable.set(static_cast<int32_t>(valueNode.asInt64()));
		}
	}
}

void SettingsManager::registerSetting(const std::string& key, flx::StringObservable& observable) {
	m_registeredSettings[key] = {Setting::Type::STRING, &observable};

	// Subscribe to changes to trigger save
	observable.subscribe([this](const std::string&) {
		this->triggerSave();
	});

	// If we have cached settings, apply the value now
	if (m_cachedSettings) {
		const flx::core::FlxValueView valueNode = m_cachedSettings->root().child(key);
		if (valueNode.valid() && valueNode.hasValue()) {
			const std::string value = valueNode.asString();
			observable.set(value.c_str());
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
				m_cachedSettings = flx::core::FlxValueDocument::parseJson(std::string(buf));
			}
			free(buf);
		}
	}
	fclose(f);
	Log::info(TAG, "Settings loaded and cached");
}

void SettingsManager::saveSettings() {
	// Build JSON string manually
	std::string jsonStr = "{";
	bool first = true;

	for (auto const& [key, setting]: m_registeredSettings) {
		if (!first) {
			jsonStr += ",";
		}
		first = false;

		// Add escaped key
		jsonStr += "\"";
		appendEscapedJsonString(key, jsonStr);
		jsonStr += "\":";

		if (setting.type == Setting::Type::INT) {
			auto* obs = (flx::Observable<int32_t>*)setting.observable;
			jsonStr += std::to_string(obs->get());
		} else {
			auto* obs = (flx::StringObservable*)setting.observable;
			std::string val = obs->get();
			// Escape string for JSON
			jsonStr += "\"";
			appendEscapedJsonString(val, jsonStr);
			jsonStr += "\"";
		}
	}

	jsonStr += "}";

	FILE* f = fopen(SETTINGS_TMP_PATH, "w");
	if (f) {
		fprintf(f, "%s", jsonStr.c_str());
		fsync(fileno(f));
		fclose(f);
		unlink(SETTINGS_PATH);
		rename(SETTINGS_TMP_PATH, SETTINGS_PATH);
		Log::info(TAG, "Settings saved successfully");

		// Also update cache
		m_cachedSettings = flx::core::FlxValueDocument::parseJson(jsonStr);
	} else {
		Log::error(TAG, "Failed to open settings file for writing");
		flx::core::Bundle data;
		data.putString("title", "Settings Error");
		data.putString("message", "Failed to save preferences");
		data.putString("appName", "System");
		data.putString("icon", "save");
		data.putInt32("priority", 2);
		flx::core::EventBus::getInstance().publish("system.notify", data);
	}
}

} // namespace flx::system
