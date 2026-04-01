#include <algorithm>
#include <cJSON.h>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <flx/core/Logger.hpp>
#include <flx/system/managers/SettingsManager.hpp>
#include <flx/system/managers/WallpaperManager.hpp>

static constexpr const char* TAG = "WallpaperManager";

namespace flx::system {

namespace {

cJSON* parseJsonValue(const std::string& value) {
	if (value == "true") {
		return cJSON_CreateBool(1);
	}
	if (value == "false") {
		return cJSON_CreateBool(0);
	}

	char* end_ptr = nullptr;
	errno = 0;
	long const maybe_int = std::strtol(value.c_str(), &end_ptr, 10);
	if (errno == 0 && end_ptr != value.c_str() && *end_ptr == '\0') {
		return cJSON_CreateNumber(static_cast<double>(maybe_int));
	}

	end_ptr = nullptr;
	errno = 0;
	double const maybe_double = std::strtod(value.c_str(), &end_ptr);
	if (errno == 0 && end_ptr != value.c_str() && *end_ptr == '\0') {
		return cJSON_CreateNumber(maybe_double);
	}

	return cJSON_CreateString(value.c_str());
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

void WallpaperManager::applyEffect(const std::string& key, const std::string& value) {
	if (key.empty()) {
		return;
	}

	std::string const current = m_wallpaper_effects_subject.get();
	cJSON* root = cJSON_Parse(current.c_str());
	if (root == nullptr || !cJSON_IsObject(root)) {
		if (root != nullptr) {
			cJSON_Delete(root);
		}
		root = cJSON_CreateObject();
	}

	cJSON* item = parseJsonValue(value);
	if (item != nullptr) {
		cJSON_DeleteItemFromObject(root, key.c_str());
		cJSON_AddItemToObject(root, key.c_str(), item);
	}

	char* out = cJSON_PrintUnformatted(root);
	if (out != nullptr) {
		m_wallpaper_effects_subject.set(out);
		cJSON_free(out);
	}

	cJSON_Delete(root);
}

void WallpaperManager::removeEffect(const std::string& key) {
	if (key.empty()) {
		return;
	}

	std::string const current = m_wallpaper_effects_subject.get();
	cJSON* root = cJSON_Parse(current.c_str());
	if (root == nullptr || !cJSON_IsObject(root)) {
		if (root != nullptr) {
			cJSON_Delete(root);
		}
		root = cJSON_CreateObject();
	}

	cJSON_DeleteItemFromObject(root, key.c_str());

	char* out = cJSON_PrintUnformatted(root);
	if (out != nullptr) {
		m_wallpaper_effects_subject.set(out);
		cJSON_free(out);
	}

	cJSON_Delete(root);
}

flx::Observable<int32_t>& WallpaperManager::getWallpaperEnabledObservable() {
	return m_wallpaper_enabled_subject;
}

flx::StringObservable& WallpaperManager::getWallpaperSourceObservable() {
	return m_wallpaper_source_subject;
}

} // namespace flx::system
