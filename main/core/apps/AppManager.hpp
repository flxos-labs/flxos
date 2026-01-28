#pragma once

#include "core/tasks/TaskManager.hpp"
#include <memory>
#include <string>
#include <vector>

namespace System {
namespace Apps {
class App {
public:

	virtual ~App() = default;

	virtual void onStart() {}
	virtual void onResume() {}
	virtual void onPause() {}
	virtual void onStop() {}
	virtual void update() {}

	virtual std::string getPackageName() const = 0;
	virtual std::string getAppName() const = 0;
	virtual const void* getIcon() const { return nullptr; }
	virtual void createUI(void* parent) {}
	virtual std::string getVersion() const { return "1.0.0"; }

	bool isActive() const { return m_isActive; }
	void setActive(bool active) { m_isActive = active; }

protected:

	bool m_isActive = false;
};

class AppManager {
public:

	static AppManager& getInstance();

	void init();
	void registerApp(std::shared_ptr<App> app);
	const std::vector<std::shared_ptr<App>>& getInstalledApps() const;

	void startApp(std::shared_ptr<App> app);
	void startApp(const std::string& packageName);
	void stopApp(const std::string& packageName);
	void stopApp(std::shared_ptr<App> app);
	void stopCurrentApp();

	std::shared_ptr<App> getAppByPackageName(const std::string& packageName);
	std::shared_ptr<App> getCurrentApp() const;

	void update();

private:

	AppManager();
	~AppManager() = default;
	AppManager(const AppManager&) = delete;
	AppManager& operator=(const AppManager&) = delete;

	std::shared_ptr<App> m_currentApp;
	std::vector<std::shared_ptr<App>> m_apps;

	void* m_mutex = nullptr;
	void* m_executor = nullptr;
};

} // namespace Apps
} // namespace System