#pragma once

#include <flx/core/Observable.hpp>
#include <flx/core/Singleton.hpp>
#include "core/services/IService.hpp"
#include "core/services/ServiceManifest.hpp"
#include <functional>
#include <map>
#include <memory>
#include <string>

#include "esp_timer.h"

namespace System {

class SettingsManager : public flx::Singleton<SettingsManager>, public Services::IService {
	friend class flx::Singleton<SettingsManager>;

public:

	// ──── IService manifest ────
	static const Services::ServiceManifest serviceManifest;
	const Services::ServiceManifest& getManifest() const override { return serviceManifest; }

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
	void* m_json_cache = nullptr; // cJSON*

	esp_timer_handle_t m_save_timer = nullptr;
};

} // namespace System
