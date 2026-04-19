#pragma once

#include <flx/core/Observable.hpp>
#include <flx/core/Singleton.hpp>
#include <flx/core/Value.hpp>
#include <flx/services/IService.hpp>
#include <flx/services/ServiceManifest.hpp>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>

#include "esp_timer.h"

namespace flx::system {

class SettingsManager : public flx::Singleton<SettingsManager>, public flx::services::IService {
	friend class flx::Singleton<SettingsManager>;

public:

	// ──── IService manifest ────
	static const flx::services::ServiceManifest serviceManifest;
	const flx::services::ServiceManifest& getManifest() const override { return serviceManifest; }

	// ──── IService lifecycle ────
	bool onStart() override;
	void onStop() override;

	// Registration
	void registerSetting(const std::string& key, flx::Observable<int32_t>& observable);
	void registerSetting(const std::string& key, flx::StringObservable& observable);

	void triggerSave();
	void saveSettings();
	void loadSettings();

private:

	SettingsManager() = default;
	~SettingsManager() = default;

	struct Setting {
		enum class Type { INT,
			STRING } type;
		void* observable;
	};

	std::map<std::string, Setting> m_registeredSettings {};
	std::optional<flx::core::FlxValueDocument> m_cachedSettings {};

	esp_timer_handle_t m_save_timer = nullptr;
};

} // namespace flx::system
