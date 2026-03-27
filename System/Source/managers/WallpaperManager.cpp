#include <algorithm>
#include <cmath>
#include <cstdint>
#include <flx/core/Logger.hpp>
#include <flx/system/managers/SettingsManager.hpp>
#include <flx/system/managers/WallpaperManager.hpp>

static constexpr const char* TAG = "WallpaperManager";

namespace flx::system {

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

void WallpaperManager::setWallpaper(const std::string& source, const std::string& type) {
	m_wallpaper_type_subject.set(type.c_str());
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

void WallpaperManager::applyEffect(const std::string& effect_name, const std::string& params_json) {
	// The effects observable stores a simple JSON-like string.
	// We use a minimal key-value update: if the effect key already exists we replace it.
	// Format: {"blur":5,"brightness":0.8,...}
	std::string effects = m_wallpaper_effects_subject.get();
	if (effects.empty() || effects == "{}") {
		effects = "{\"" + effect_name + "\":" + params_json + "}";
	} else {
		// Remove trailing '}'
		if (!effects.empty() && effects.back() == '}') {
			effects.pop_back();
		}
		// Check if key already present (simple search)
		std::string key = "\"" + effect_name + "\":";
		auto pos = effects.find(key);
		if (pos != std::string::npos) {
			// Find end of existing value
			size_t val_start = pos + key.size();
			size_t val_end = effects.find_first_of(",}", val_start);
			if (val_end == std::string::npos) {
				val_end = effects.size();
			}
			effects = effects.substr(0, val_start) + params_json + effects.substr(val_end) + "}";
		} else {
			effects += ",\"" + effect_name + "\":" + params_json + "}";
		}
	}
	m_wallpaper_effects_subject.set(effects.c_str());
	Log::info(TAG, "Applied effect '%s': %s", effect_name.c_str(), params_json.c_str());
}

void WallpaperManager::removeEffect(const std::string& effect_name) {
	std::string effects = m_wallpaper_effects_subject.get();
	if (effects.empty() || effects == "{}") {
		return;
	}
	std::string key = "\"" + effect_name + "\":";
	auto pos = effects.find(key);
	if (pos == std::string::npos) {
		return;
	}
	// Find start of key (may be preceded by '{' or ',')
	size_t key_start = pos;
	if (key_start > 0 && effects[key_start - 1] == ',') {
		--key_start;
	}
	// Find end of value
	size_t val_start = pos + key.size();
	size_t val_end = effects.find_first_of(",}", val_start);
	if (val_end == std::string::npos) {
		val_end = effects.size();
	}
	// If no preceding comma, remove trailing comma if present
	size_t remove_end = val_end;
	if (key_start == pos && val_end < effects.size() && effects[val_end] == ',') {
		remove_end = val_end + 1;
	}
	effects = effects.substr(0, key_start) + effects.substr(remove_end);
	if (effects.empty()) {
		effects = "{}";
	}
	m_wallpaper_effects_subject.set(effects.c_str());
	Log::info(TAG, "Removed effect '%s'", effect_name.c_str());
}

flx::Observable<int32_t>& WallpaperManager::getWallpaperEnabledObservable() {
	return m_wallpaper_enabled_subject;
}

flx::StringObservable& WallpaperManager::getWallpaperSourceObservable() {
	return m_wallpaper_source_subject;
}

} // namespace flx::system
