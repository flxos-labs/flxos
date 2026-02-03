#include "AppManager.hpp"
#include "core/common/Logger.hpp"
#include "core/tasks/gui/GuiTask.hpp"
#include "core/ui/desktop/Desktop.hpp"
#include "files/FilesApp.hpp"
#include "freertos/semphr.h"
#include "settings/SettingsApp.hpp"
#include "system_info/SystemInfoApp.hpp"

namespace System::Apps {
class AppExecutor : public System::Task {
public:

	AppExecutor() : System::Task("app_executor", 8 * 1024, 4) {
		setRestartPolicy(RestartPolicy::RESTART_TASK);
	}

protected:

	void run(void*) override {
		setWatchdogTimeout(10000);
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
	Log::info("AppManager", "Initializing AppManager...");
	registerApp(std::make_shared<SettingsApp>());
	registerApp(std::make_shared<FilesApp>());
	registerApp(std::make_shared<SystemInfoApp>());

	if (!m_executor) {
		Log::info("AppManager", "Starting AppExecutor task...");
		m_executor = new AppExecutor();
		static_cast<AppExecutor*>(m_executor)->start();
	}
}

void AppManager::registerApp(std::shared_ptr<App> app) {
	if (!app) {
		return;
	}
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);
	for (const auto& ex: m_apps)
		if (ex->getPackageName() == app->getPackageName()) {
			xSemaphoreGive((SemaphoreHandle_t)m_mutex);
			return;
		}
	Log::info("AppManager", "Registered app: %s (%s)", app->getAppName().c_str(), app->getPackageName().c_str());
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
		return false;
	}

	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);

	if (m_currentApp == app) {
		xSemaphoreGive((SemaphoreHandle_t)m_mutex);
		return true;
	}

	std::string packageName = app->getPackageName();

	GuiTask::lock();

	// Pause current app if exists
	if (m_currentApp) {
		Log::info("AppManager", "Pausing app: %s", m_currentApp->getPackageName().c_str());
		try {
			m_currentApp->onPause();
		} catch (const std::exception& e) {
			Log::error("AppManager", "Error pausing app: %s", e.what());
		} catch (...) {
			Log::error("AppManager", "Unknown error pausing app");
		}
	}

	m_currentApp = app;
	bool success = true;

	// Start the new app with crash recovery
	try {
		if (!m_currentApp->isActive()) {
			Log::info("AppManager", "Starting app: %s", packageName.c_str());
			m_currentApp->onStart();
			m_currentApp->setActive(true);
		}
		Log::info("AppManager", "Resuming app: %s", packageName.c_str());
		m_currentApp->onResume();
	} catch (const std::exception& e) {
		Log::error("AppManager", "CRASH in app %s: %s", packageName.c_str(), e.what());
		m_currentApp->setActive(false);
		m_currentApp = nullptr;
		success = false;
	} catch (...) {
		Log::error("AppManager", "CRASH in app %s (unknown error)", packageName.c_str());
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
		return false;
	}

	std::string pkg = app->getPackageName();

	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);
	bool was_active = app->isActive() || (m_currentApp == app);

	if (m_currentApp == app) {
		Log::info("AppManager", "Stopping current app: %s", pkg.c_str());
		GuiTask::lock();
		try {
			m_currentApp->onPause();
		} catch (const std::exception& e) {
			Log::error("AppManager", "Error onPause while stopping: %s", e.what());
		} catch (...) {
		}
		try {
			m_currentApp->onStop();
		} catch (const std::exception& e) {
			Log::error("AppManager", "Error onStop while stopping: %s", e.what());
		} catch (...) {
		}
		m_currentApp->setActive(false);
		m_currentApp = nullptr;
		GuiTask::unlock();
	} else if (app->isActive()) {
		Log::info("AppManager", "Stopping inactive but active-state app: %s", pkg.c_str());
		GuiTask::lock();
		try {
			app->onStop();
		} catch (const std::exception& e) {
			Log::error("AppManager", "Error onStop: %s", e.what());
		} catch (...) {
		}
		app->setActive(false);
		GuiTask::unlock();
	} else {
	}
	xSemaphoreGive((SemaphoreHandle_t)m_mutex);

	if (was_active) {
		notifyAppStopped(pkg);
		if (closeUI) {
			GuiTask::lock();
			Desktop::getInstance().closeApp(pkg);
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
		GuiTask::lock();
		try {
			app->update();
		} catch (const std::exception& e) {
		} catch (...) {
		}
		GuiTask::unlock();
	}
}

bool AppManager::startApp(const std::string& pkg) {
	auto app = getAppByPackageName(pkg);
	if (!app) {
		return false;
	}
	return startApp(app);
}

bool AppManager::stopApp(const std::string& pkg, bool closeUI) {
	auto app = getAppByPackageName(pkg);
	if (!app) {
		return false;
	}
	return stopApp(app, closeUI);
}

void AppManager::stopCurrentApp() {
	if (m_currentApp) {
		stopApp(m_currentApp);
	} else {
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
		return;
	}
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);
	// Check if already added
	for (auto* obs: m_observers) {
		if (obs == observer) {
			xSemaphoreGive((SemaphoreHandle_t)m_mutex);
			return;
		}
	}
	m_observers.push_back(observer);
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
			break;
		}
	}
	xSemaphoreGive((SemaphoreHandle_t)m_mutex);
}

void AppManager::notifyAppStarted(const std::string& packageName) {
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);
	auto observers = m_observers; // Copy to avoid holding lock during callbacks
	xSemaphoreGive((SemaphoreHandle_t)m_mutex);

	for (auto* observer: observers) {
		try {
			observer->onAppStarted(packageName);
		} catch (const std::exception& e) {
		} catch (...) {
		}
	}
}

void AppManager::notifyAppStopped(const std::string& packageName) {
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);
	auto observers = m_observers; // Copy to avoid holding lock during callbacks
	xSemaphoreGive((SemaphoreHandle_t)m_mutex);

	for (auto* observer: observers) {
		try {
			observer->onAppStopped(packageName);
		} catch (const std::exception& e) {
		} catch (...) {
		}
	}
}

void AppManager::performHealthCheck() {
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);

	// Check for inconsistencies
	int active_count = 0;
	for (const auto& app: m_apps) {
		if (app->isActive()) {
			active_count++;
			if (app != m_currentApp) {
			}
		}
	}

	if (m_currentApp && !m_currentApp->isActive()) {
	}

	if (active_count > 1) {
	}

	xSemaphoreGive((SemaphoreHandle_t)m_mutex);
}

} // namespace System::Apps
