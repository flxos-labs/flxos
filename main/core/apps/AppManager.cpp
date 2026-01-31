#include "AppManager.hpp"
#include "core/tasks/gui/GuiTask.hpp"
#include "core/ui/DE/DE.hpp"
#include "esp_log.h"
#include "files/FilesApp.hpp"
#include "freertos/semphr.h"
#include "settings/SettingsApp.hpp"
#include "system_info/SystemInfoApp.hpp"

static const char* TAG = "AppManager";

namespace System::Apps {
class AppExecutor : public System::Task {
public:

	AppExecutor() : System::Task("app_executor", 8 * 1024, 4) {
		setRestartPolicy(RestartPolicy::RESTART_TASK);
	}

protected:

	void run(void*) override {
		setWatchdogTimeout(10000);
		ESP_LOGI("AppExecutor", "AppExecutor task started");
		while (true) {
			heartbeat();
			AppManager::getInstance().update();
			vTaskDelay(pdMS_TO_TICKS(16));
		}
	}
};

AppManager& AppManager::getInstance() {
	static AppManager instance;
	return instance;
}
AppManager::AppManager() { m_mutex = xSemaphoreCreateMutex(); }

void AppManager::init() {
	ESP_LOGI(TAG, "Initializing AppManager...");
	registerApp(std::make_shared<SettingsApp>());
	registerApp(std::make_shared<FilesApp>());
	registerApp(std::make_shared<SystemInfoApp>());

	if (!m_executor) {
		ESP_LOGD(TAG, "Starting AppExecutor task");
		m_executor = new AppExecutor();
		static_cast<AppExecutor*>(m_executor)->start();
	}
}

void AppManager::registerApp(std::shared_ptr<App> app) {
	if (!app) {
		ESP_LOGW(TAG, "Attempted to register null app");
		return;
	}
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);
	for (const auto& ex: m_apps)
		if (ex->getPackageName() == app->getPackageName()) {
			ESP_LOGW(TAG, "App '%s' already registered", app->getPackageName().c_str());
			xSemaphoreGive((SemaphoreHandle_t)m_mutex);
			return;
		}
	ESP_LOGI(TAG, "Registering app: %s", app->getAppName().c_str());
	m_apps.push_back(app);
	xSemaphoreGive((SemaphoreHandle_t)m_mutex);
}

std::shared_ptr<App> AppManager::getAppByPackageName(const std::string& pkg) {
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);
	std::shared_ptr<App> found = nullptr;
	for (auto& app: m_apps)
		if (app->getPackageName() == pkg) {
			found = app;
			break;
		}
	xSemaphoreGive((SemaphoreHandle_t)m_mutex);
	return found;
}

bool AppManager::isAppRegistered(const std::string& packageName) const {
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);
	bool found = false;
	for (const auto& app: m_apps) {
		if (app->getPackageName() == packageName) {
			found = true;
			break;
		}
	}
	xSemaphoreGive((SemaphoreHandle_t)m_mutex);
	return found;
}

bool AppManager::startApp(std::shared_ptr<App> app) {
	if (!app) {
		ESP_LOGE(TAG, "Cannot start null app");
		return false;
	}

	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);

	if (m_currentApp == app) {
		ESP_LOGD(TAG, "App '%s' is already the current app", app->getAppName().c_str());
		xSemaphoreGive((SemaphoreHandle_t)m_mutex);
		return true;
	}

	ESP_LOGI(TAG, "Starting app: %s", app->getAppName().c_str());
	std::string packageName = app->getPackageName();

	GuiTask::lock();

	// Pause current app if exists
	if (m_currentApp) {
		ESP_LOGD(TAG, "Pausing current app: %s", m_currentApp->getAppName().c_str());
		try {
			m_currentApp->onPause();
		} catch (const std::exception& e) {
			ESP_LOGE(TAG, "Exception in onPause for app %s: %s", m_currentApp->getAppName().c_str(), e.what());
		} catch (...) {
			ESP_LOGE(TAG, "Unknown exception in onPause for app: %s", m_currentApp->getAppName().c_str());
		}
	}

	m_currentApp = app;
	bool success = true;

	// Start the new app with crash recovery
	try {
		if (!m_currentApp->isActive()) {
			ESP_LOGD(TAG, "Calling onStart for app: %s", m_currentApp->getAppName().c_str());
			m_currentApp->onStart();
			m_currentApp->setActive(true);
		}
		ESP_LOGD(TAG, "Calling onResume for app: %s", m_currentApp->getAppName().c_str());
		m_currentApp->onResume();
	} catch (const std::exception& e) {
		ESP_LOGE(TAG, "Exception during app start/resume %s: %s", m_currentApp->getAppName().c_str(), e.what());
		m_currentApp->setActive(false);
		m_currentApp = nullptr;
		success = false;
	} catch (...) {
		ESP_LOGE(TAG, "Unknown exception during app start/resume: %s", m_currentApp->getAppName().c_str());
		m_currentApp->setActive(false);
		m_currentApp = nullptr;
		success = false;
	}

	GuiTask::unlock();
	xSemaphoreGive((SemaphoreHandle_t)m_mutex);

	if (success) {
		notifyAppStarted(packageName);
	}

	return success;
}

bool AppManager::stopApp(std::shared_ptr<App> app, bool closeUI) {
	if (!app) {
		ESP_LOGW(TAG, "Attempted to stop a null app");
		return false;
	}

	std::string pkg = app->getPackageName();
	ESP_LOGI(TAG, "Request to stop app: %s", app->getAppName().c_str());

	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);
	bool was_active = app->isActive() || (m_currentApp == app);

	if (m_currentApp == app) {
		ESP_LOGD(TAG, "Stopping current app: %s", m_currentApp->getAppName().c_str());
		GuiTask::lock();
		try {
			m_currentApp->onPause();
		} catch (const std::exception& e) {
			ESP_LOGE(TAG, "Exception in onPause for app %s: %s", m_currentApp->getAppName().c_str(), e.what());
		} catch (...) {
			ESP_LOGE(TAG, "Unknown exception in onPause for app: %s", m_currentApp->getAppName().c_str());
		}
		try {
			m_currentApp->onStop();
		} catch (const std::exception& e) {
			ESP_LOGE(TAG, "Exception in onStop for app %s: %s", m_currentApp->getAppName().c_str(), e.what());
		} catch (...) {
			ESP_LOGE(TAG, "Unknown exception in onStop for app: %s", m_currentApp->getAppName().c_str());
		}
		m_currentApp->setActive(false);
		m_currentApp = nullptr;
		GuiTask::unlock();
	} else if (app->isActive()) {
		ESP_LOGD(TAG, "Stopping background app: %s", app->getAppName().c_str());
		GuiTask::lock();
		try {
			app->onStop();
		} catch (const std::exception& e) {
			ESP_LOGE(TAG, "Exception in onStop for app %s: %s", app->getAppName().c_str(), e.what());
		} catch (...) {
			ESP_LOGE(TAG, "Unknown exception in onStop for app: %s", app->getAppName().c_str());
		}
		app->setActive(false);
		GuiTask::unlock();
	} else {
		ESP_LOGD(TAG, "App %s is not active, no action taken", app->getAppName().c_str());
	}
	xSemaphoreGive((SemaphoreHandle_t)m_mutex);

	if (was_active) {
		notifyAppStopped(pkg);
		if (closeUI) {
			GuiTask::lock();
			DE::getInstance().closeApp(pkg);
			GuiTask::unlock();
		}
	}

	return true;
}

void AppManager::update() {
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);
	auto app = m_currentApp;
	xSemaphoreGive((SemaphoreHandle_t)m_mutex);

	if (app) {
		ESP_LOGD(TAG, "Updating active app: %s", app->getAppName().c_str());
		GuiTask::lock();
		try {
			app->update();
		} catch (const std::exception& e) {
			ESP_LOGE(TAG, "Exception in update for app %s: %s", app->getAppName().c_str(), e.what());
		} catch (...) {
			ESP_LOGE(TAG, "Unknown exception in update for app: %s", app->getAppName().c_str());
		}
		GuiTask::unlock();
	}
}

bool AppManager::startApp(const std::string& pkg) {
	auto app = getAppByPackageName(pkg);
	if (!app) {
		ESP_LOGE(TAG, "Cannot start app '%s': not registered", pkg.c_str());
		return false;
	}
	return startApp(app);
}

bool AppManager::stopApp(const std::string& pkg, bool closeUI) {
	auto app = getAppByPackageName(pkg);
	if (!app) {
		ESP_LOGW(TAG, "Cannot stop app '%s': not found", pkg.c_str());
		return false;
	}
	return stopApp(app, closeUI);
}

void AppManager::stopCurrentApp() {
	if (m_currentApp) {
		stopApp(m_currentApp);
	} else {
		ESP_LOGD(TAG, "No current app to stop");
	}
}

const std::vector<std::shared_ptr<App>>& AppManager::getInstalledApps() const {
	return m_apps;
}

std::shared_ptr<App> AppManager::getCurrentApp() const {
	return m_currentApp;
}

void AppManager::addObserver(AppStateObserver* observer) {
	if (!observer) {
		ESP_LOGW(TAG, "Attempted to add null observer");
		return;
	}
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);
	// Check if already added
	for (auto* obs: m_observers) {
		if (obs == observer) {
			ESP_LOGD(TAG, "Observer already registered");
			xSemaphoreGive((SemaphoreHandle_t)m_mutex);
			return;
		}
	}
	m_observers.push_back(observer);
	ESP_LOGI(TAG, "Observer added, total observers: %d", m_observers.size());
	xSemaphoreGive((SemaphoreHandle_t)m_mutex);
}

void AppManager::removeObserver(AppStateObserver* observer) {
	if (!observer) {
		return;
	}
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);
	for (auto it = m_observers.begin(); it != m_observers.end(); ++it) {
		if (*it == observer) {
			m_observers.erase(it);
			ESP_LOGI(TAG, "Observer removed, total observers: %d", m_observers.size());
			break;
		}
	}
	xSemaphoreGive((SemaphoreHandle_t)m_mutex);
}

void AppManager::notifyAppStarted(const std::string& packageName) {
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);
	auto observers = m_observers; // Copy to avoid holding lock during callbacks
	xSemaphoreGive((SemaphoreHandle_t)m_mutex);

	ESP_LOGD(TAG, "Notifying %d observers: app started '%s'", observers.size(), packageName.c_str());
	for (auto* observer: observers) {
		try {
			observer->onAppStarted(packageName);
		} catch (const std::exception& e) {
			ESP_LOGE(TAG, "Exception in observer onAppStarted callback: %s", e.what());
		} catch (...) {
			ESP_LOGE(TAG, "Unknown exception in observer onAppStarted callback");
		}
	}
}

void AppManager::notifyAppStopped(const std::string& packageName) {
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);
	auto observers = m_observers; // Copy to avoid holding lock during callbacks
	xSemaphoreGive((SemaphoreHandle_t)m_mutex);

	ESP_LOGD(TAG, "Notifying %d observers: app stopped '%s'", observers.size(), packageName.c_str());
	for (auto* observer: observers) {
		try {
			observer->onAppStopped(packageName);
		} catch (const std::exception& e) {
			ESP_LOGE(TAG, "Exception in observer onAppStopped callback: %s", e.what());
		} catch (...) {
			ESP_LOGE(TAG, "Unknown exception in observer onAppStopped callback");
		}
	}
}

void AppManager::performHealthCheck() {
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);

	ESP_LOGI(TAG, "=== App Manager Health Check ===");
	ESP_LOGI(TAG, "Total registered apps: %d", m_apps.size());
	ESP_LOGI(TAG, "Current app: %s", m_currentApp ? m_currentApp->getAppName().c_str() : "None");
	ESP_LOGI(TAG, "Total observers: %d", m_observers.size());

	// Check for inconsistencies
	int active_count = 0;
	for (const auto& app: m_apps) {
		if (app->isActive()) {
			active_count++;
			ESP_LOGI(TAG, "  Active app: %s", app->getAppName().c_str());
			if (app != m_currentApp) {
				ESP_LOGW(TAG, "  WARNING: App '%s' is marked active but is not current app", app->getAppName().c_str());
			}
		}
	}

	if (m_currentApp && !m_currentApp->isActive()) {
		ESP_LOGW(TAG, "WARNING: Current app '%s' is not marked as active", m_currentApp->getAppName().c_str());
	}

	if (active_count > 1) {
		ESP_LOGW(TAG, "WARNING: Multiple apps marked as active (%d)", active_count);
	}

	ESP_LOGI(TAG, "=== Health Check Complete ===");

	xSemaphoreGive((SemaphoreHandle_t)m_mutex);
}

} // namespace System::Apps
