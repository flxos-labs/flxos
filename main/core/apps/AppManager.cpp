#include "AppManager.hpp"
#include "core/tasks/gui/GuiTask.hpp"
#include "core/ui/DE/DE.hpp"
#include "esp_log.h"
#include "files/FilesApp.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "settings/SettingsApp.hpp"

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
	registerApp(std::make_shared<SettingsApp>());
	registerApp(std::make_shared<FilesApp>());
	if (!m_executor) {
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
			xSemaphoreGive((SemaphoreHandle_t)m_mutex);
			return;
		}
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
		GuiTask::lock();
		if (m_currentApp)
			m_currentApp->onPause();
		m_currentApp = app;
		if (m_currentApp) {
			if (!m_currentApp->isActive()) {
				m_currentApp->onStart();
				m_currentApp->setActive(true);
			}
			m_currentApp->onResume();
		}
		GuiTask::unlock();
	}
	xSemaphoreGive((SemaphoreHandle_t)m_mutex);
}

void AppManager::stopApp(std::shared_ptr<App> app) {
	if (!app)
		return;
	std::string pkg = app->getPackageName();
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);
	bool active = app->isActive() || (m_currentApp == app);

	GuiTask::lock();
	if (m_currentApp == app) {
		m_currentApp->onPause();
		m_currentApp->onStop();
		m_currentApp->setActive(false);
		m_currentApp = nullptr;
	} else if (app->isActive()) {
		app->onStop();
		app->setActive(false);
	}
	GuiTask::unlock();

	xSemaphoreGive((SemaphoreHandle_t)m_mutex);
	if (active) {
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
		GuiTask::lock();
		app->update();
		GuiTask::unlock();
	}
}

void AppManager::startApp(const std::string& pkg) {
	if (auto a = getAppByPackageName(pkg))
		startApp(a);
}
void AppManager::stopApp(const std::string& pkg) {
	stopApp(getAppByPackageName(pkg));
}
void AppManager::stopCurrentApp() { stopApp(m_currentApp); }
const std::vector<std::shared_ptr<App>>& AppManager::getInstalledApps() const {
	return m_apps;
}
std::shared_ptr<App> AppManager::getCurrentApp() const { return m_currentApp; }

} // namespace System::Apps