#pragma once

#include <flx/core/Observable.hpp>
#include <flx/core/Singleton.hpp>
#include <flx/services/IService.hpp>
#include <flx/services/ServiceManifest.hpp>
#include <string>

namespace flx::system {

class WallpaperManager : public flx::Singleton<WallpaperManager>, public flx::services::IService {
	friend class flx::Singleton<WallpaperManager>;

public:

	static const flx::services::ServiceManifest serviceManifest;
	const flx::services::ServiceManifest& getManifest() const override { return serviceManifest; }

	bool onStart() override;
	void onStop() override;

	void onFrame(uint32_t delta_ms);

	void setWallpaper(const std::string& source, const std::string& type);
	void setAnimationSpeed(int32_t speed);
	void setQualityLevel(int32_t level);
	void applyEffect(const std::string& effect_name, const std::string& params_json);
	void removeEffect(const std::string& effect_name);

	flx::Observable<int32_t>& getWallpaperEnabledObservable();
	flx::StringObservable& getWallpaperSourceObservable();
	flx::StringObservable& getWallpaperTypeObservable() { return m_wallpaper_type_subject; }
	flx::StringObservable& getWallpaperEffectsObservable() { return m_wallpaper_effects_subject; }
	flx::Observable<int32_t>& getAnimationSpeedObservable() { return m_animation_speed_subject; }
	flx::Observable<int32_t>& getQualityLevelObservable() { return m_quality_level_subject; }
	flx::Observable<int32_t>& getBenchmarkEnabledObservable() { return m_benchmark_enabled_subject; }
	flx::Observable<int32_t>& getCpuUsageObservable() { return m_cpu_usage_subject; }

private:

	WallpaperManager() = default;
	~WallpaperManager() = default;

	flx::Observable<int32_t> m_wallpaper_enabled_subject {0};
	flx::StringObservable m_wallpaper_source_subject {""};
	flx::StringObservable m_wallpaper_type_subject {"static"};
	flx::StringObservable m_wallpaper_effects_subject {"{}"};
	flx::Observable<int32_t> m_animation_speed_subject {50};
	flx::Observable<int32_t> m_quality_level_subject {1};
	flx::Observable<int32_t> m_benchmark_enabled_subject {0};
	flx::Observable<int32_t> m_cpu_usage_subject {0};
	float m_frame_time_ema_ms = 0.0f;
};

} // namespace flx::system
