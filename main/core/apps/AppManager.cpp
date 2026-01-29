#include "AppManager.hpp"
#include "core/tasks/gui/GuiTask.hpp"
#include "core/ui/DE/DE.hpp"
#include "esp_log.h"
#include "files/FilesApp.hpp"
#include "freertos/semphr.h"
#include "settings/SettingsApp.hpp"

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
	if (!m_executor) {
		ESP_LOGD(TAG, "Starting AppExecutor task");
		m_executor = new AppExecutor();
		static_cast<AppExecutor*>(m_executor)->start();
	}
}

void AppManager::registerApp(std::shared_ptr<App> app) {
	if (!app)
		return;
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

void AppManager::startApp(std::shared_ptr<App> app) {
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);
	if (m_currentApp != app) {
		ESP_LOGI(TAG, "Starting app: %s", app ? app->getAppName().c_str() : "None");
		GuiTask::lock();
		if (m_currentApp) {
			ESP_LOGD(TAG, "Pausing current app: %s", m_currentApp->getAppName().c_str());
			m_currentApp->onPause();
		}
		m_currentApp = app;
		if (m_currentApp) {
			if (!m_currentApp->isActive()) {
				ESP_LOGD(TAG, "Calling onStart for app: %s", m_currentApp->getAppName().c_str());
				m_currentApp->onStart();
				m_currentApp->setActive(true);
			}
			ESP_LOGD(TAG, "Calling onResume for app: %s", m_currentApp->getAppName().c_str());
			m_currentApp->onResume();
		}
		GuiTask::unlock();
	}
	xSemaphoreGive((SemaphoreHandle_t)m_mutex);
}

void AppManager::stopApp(std::shared_ptr<App> app, bool closeUI) {
	ESP_LOGI(TAG, "Request to stop app: %s", app ? app->getAppName().c_str() : "NULL");
	if (!app) {
		ESP_LOGW(TAG, "Attempted to stop a NULL app.");
		return;
	}
	std::string pkg = app->getPackageName();
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);
	bool was_active = app->isActive() || (m_currentApp == app);

	if (m_currentApp == app) {
		ESP_LOGD(TAG, "Stopping current app: %s", m_currentApp->getAppName().c_str());
		GuiTask::lock();
		m_currentApp->onPause(); // Call onPause before onStop for current app
		m_currentApp->onStop();
		m_currentApp->setActive(false);
		m_currentApp = nullptr;
		GuiTask::unlock();
	} else if (app->isActive()) {
		ESP_LOGD(TAG, "Stopping background app: %s", app->getAppName().c_str());
		GuiTask::lock();
		app->onStop();
		app->setActive(false);
		GuiTask::unlock();
	} else {
		ESP_LOGD(TAG, "App %s is not active, no action taken.", app->getAppName().c_str());
	}
	xSemaphoreGive((SemaphoreHandle_t)m_mutex);

	if (was_active && closeUI) {
		GuiTask::lock();
		DE::getInstance().closeApp(pkg);
		GuiTask::unlock();
	}
}

void AppManager::update() {
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);
	auto app = m_currentApp;
	xSemaphoreGive((SemaphoreHandle_t)m_mutex);
	if (app) {
		ESP_LOGD(TAG, "Updating active app: %s", app->getAppName().c_str());
		GuiTask::lock();
		app->update();
		GuiTask::unlock();
	}
}

void AppManager::startApp(const std::string& pkg) {
	if (auto a = getAppByPackageName(pkg))
		startApp(a);
}
void AppManager::stopApp(const std::string& pkg, bool closeUI) {
	stopApp(getAppByPackageName(pkg), closeUI);
}
void AppManager::stopCurrentApp() { stopApp(m_currentApp); }
const std::vector<std::shared_ptr<App>>& AppManager::getInstalledApps() const {
	return m_apps;
}
std::shared_ptr<App> AppManager::getCurrentApp() const { return m_currentApp; }

} // namespace System::Apps
