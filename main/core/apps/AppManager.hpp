#pragma once

#include "core/common/Singleton.hpp"
#include "core/tasks/TaskManager.hpp"
#include <memory>
#include <string>
#include <vector>

namespace System::Apps {

// Observer interface for app state changes
class AppStateObserver {
public:

	virtual void onAppStarted(const std::string& packageName) = 0;
	virtual void onAppStopped(const std::string& packageName) = 0;
	virtual ~AppStateObserver() = default;
};
class App {
public:

	virtual ~App() = default;

	virtual bool onStart() { return true; }
	virtual bool onResume() { return true; }
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

class AppManager : public Singleton<AppManager> {
	friend class Singleton<AppManager>;

public:

	void init();
	void registerApp(std::shared_ptr<App> app);
	const std::vector<std::shared_ptr<App>>& getInstalledApps() const;

	// App lifecycle methods with error handling
	bool startApp(std::shared_ptr<App> app);
	bool startApp(const std::string& packageName);
	bool stopApp(const std::string& packageName, bool closeUI = true);
	bool stopApp(std::shared_ptr<App> app, bool closeUI = true);
	void stopCurrentApp();

	// App queries and validation
	std::shared_ptr<App> getAppByPackageName(const std::string& packageName);
	std::shared_ptr<App> getCurrentApp() const;
	bool isAppRegistered(const std::string& packageName) const;

	// Observer pattern for state synchronization
	void addObserver(AppStateObserver* observer);
	void removeObserver(AppStateObserver* observer);

	// Diagnostics and health checks
	void performHealthCheck();

	void update();

private:

	AppManager();
	~AppManager() = default;

	std::shared_ptr<App> m_currentApp {};
	std::vector<std::shared_ptr<App>> m_apps;
	std::vector<AppStateObserver*> m_observers {};

	void* m_mutex = nullptr;
	void* m_executor = nullptr;

	// Helper methods
	void notifyAppStarted(const std::string& packageName);
	void notifyAppStopped(const std::string& packageName);
};

} // namespace System::Apps
