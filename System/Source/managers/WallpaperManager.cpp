#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <flx/core/Logger.hpp>
#include <flx/core/Value.hpp>
#include <flx/system/managers/SettingsManager.hpp>
#include <flx/system/managers/WallpaperManager.hpp>

static constexpr const char* TAG = "WallpaperManager";

namespace flx::system {

namespace {

std::string jsonEscapeString(const std::string& str) {
	std::string result;
	for (char c : str) {
		if (c == '\\') result += "\\\\";
		else if (c == '\"') result += "\\\"";
		else if (c == '\n') result += "\\n";
		else if (c == '\r') result += "\\r";
		else if (c == '\t') result += "\\t";
		else result += c;
	}
	return result;
}

std::string valueToJsonValue(const std::string& value) {
	if (value == "true" || value == "false") {
		return value;
	}

	char* end_ptr = nullptr;
	errno = 0;
	long const maybe_int = std::strtol(value.c_str(), &end_ptr, 10);
	if (errno == 0 && end_ptr != value.c_str() && *end_ptr == '\0') {
		return std::to_string(maybe_int);
	}

	end_ptr = nullptr;
	errno = 0;
	double const maybe_double = std::strtod(value.c_str(), &end_ptr);
	if (errno == 0 && end_ptr != value.c_str() && *end_ptr == '\0') {
		return std::to_string(static_cast<long long>(maybe_double));
	}

	return "\"" + jsonEscapeString(value) + "\"";
}

} // namespace

const flx::services::ServiceManifest WallpaperManager::serviceManifest = {
	.serviceId = "com.flxos.wallpaper",
	.serviceName = "Wallpaper",
	.dependencies = {"com.flxos.settings"},
	.priority = 30,
	.required = false,
	.autoStart = true,
	.guiRequired = false,
	.capabilities = flx::services::ServiceCapability::None,
	.description = "Wallpaper engine coordinator and settings",
};

bool WallpaperManager::onStart() {
	SettingsManager::getInstance().registerSetting("wp_enabled", m_wallpaper_enabled_subject);
	SettingsManager::getInstance().registerSetting("wp_source", m_wallpaper_source_subject);
	SettingsManager::getInstance().registerSetting("wp_type", m_wallpaper_type_subject);
	SettingsManager::getInstance().registerSetting("wp_effects", m_wallpaper_effects_subject);
	SettingsManager::getInstance().registerSetting("wp_animation_speed", m_animation_speed_subject);
	SettingsManager::getInstance().registerSetting("wp_quality_level", m_quality_level_subject);
	SettingsManager::getInstance().registerSetting("wp_benchmark", m_benchmark_enabled_subject);
	Log::info(TAG, "Wallpaper service started");
	return true;
}

void WallpaperManager::onStop() {
	Log::info(TAG, "Wallpaper service stopped");
}

void WallpaperManager::onFrame(uint32_t delta_ms) {
	constexpr float alpha = 0.1f;
	if (m_frame_time_ema_ms <= 0.0f) {
		m_frame_time_ema_ms = static_cast<float>(delta_ms);
	} else {
		m_frame_time_ema_ms = (alpha * static_cast<float>(delta_ms)) + ((1.0f - alpha) * m_frame_time_ema_ms);
	}

	if (m_frame_time_ema_ms <= 0.0f) {
		m_cpu_usage_subject.set(0);
		return;
	}

	float const target_frame_ms = 33.33f;
	float const usage = std::min(100.0f, (m_frame_time_ema_ms / target_frame_ms) * 100.0f);
	m_cpu_usage_subject.set(static_cast<int32_t>(std::lround(usage)));
}

void WallpaperManager::setWallpaper(const std::string& source) {
	// Force static-only wallpapers.
	m_wallpaper_type_subject.set("static");
	m_wallpaper_source_subject.set(source.c_str());
	m_wallpaper_enabled_subject.set(source.empty() ? 0 : 1);
}

void WallpaperManager::setAnimationSpeed(int32_t speed) {
	speed = std::clamp(speed, static_cast<int32_t>(0), static_cast<int32_t>(100));
	m_animation_speed_subject.set(speed);
}

void WallpaperManager::setQualityLevel(int32_t level) {
	level = std::clamp(level, static_cast<int32_t>(0), static_cast<int32_t>(2));
	m_quality_level_subject.set(level);
}

void WallpaperManager::applyEffect(const std::string& key, const std::string& value) {
	if (key.empty()) {
		return;
	}

	std::string current = m_wallpaper_effects_subject.get();
	if (current.empty()) {
		current = "{}";
	}

	// Parse current effects JSON
	auto doc = flx::core::FlxValueDocument::parseJson(current);
	if (!doc || !doc->root().isMap()) {
		// If parse failed or root is not a map, start fresh
		current = "{}";
	}

	// Rebuild JSON with the new/updated effect
	std::string result = "{";
	bool first = true;

	// Copy existing effects except the one being updated
	if (doc) {
		doc->root().forEachNamedChild([&](std::string_view itemKey, const flx::core::FlxValueView& itemValue) {
			if (itemKey != key) {
				if (!first) result += ",";
				first = false;
				result += "\"" + std::string(itemKey) + "\":";
				result += itemValue.toJsonString();
			}
		});
	}

	// Add the new effect
	if (!first) result += ",";
	result += "\"" + key + "\":" + valueToJsonValue(value);
	result += "}";

		m_wallpaper_effects_subject.set(result.c_str());
}

void WallpaperManager::removeEffect(const std::string& key) {
	if (key.empty()) {
		return;
	}

	std::string current = m_wallpaper_effects_subject.get();
	if (current.empty()) {
		return;
	}

	// Parse current effects JSON
	auto doc = flx::core::FlxValueDocument::parseJson(current);
	if (!doc || !doc->root().isMap()) {
		return;
	}

	// Rebuild JSON without the effect being removed
	std::string result = "{";
	bool first = true;

	doc->root().forEachNamedChild([&](std::string_view itemKey, const flx::core::FlxValueView& itemValue) {
		if (itemKey != key) {
			if (!first) result += ",";
			first = false;
			result += "\"" + std::string(itemKey) + "\":";
			result += itemValue.toJsonString();
		}
	});

	result += "}";
		m_wallpaper_effects_subject.set(result.c_str());
}

flx::Observable<int32_t>& WallpaperManager::getWallpaperEnabledObservable() {
	return m_wallpaper_enabled_subject;
}

flx::StringObservable& WallpaperManager::getWallpaperSourceObservable() {
	return m_wallpaper_source_subject;
}

} // namespace flx::system
