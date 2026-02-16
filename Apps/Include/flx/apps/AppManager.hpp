#pragma once

#include "AppContext.hpp"
#include "Intent.hpp"
#include <flx/core/Bundle.hpp>
#include <flx/core/Singleton.hpp>
#include <flx/kernel/TaskManager.hpp>
#include <memory>
#include <string>
#include <vector>

#include "App.hpp"

namespace flx::apps {

// Forward declaration
// class App; // Removed as we include App.hpp

// UI Callbacks
using GuiLockCallback = std::function<void()>;
using GuiUnlockCallback = std::function<void()>;
using WindowOpenCallback = std::function<void(const std::string&)>;
using WindowCloseCallback = std::function<void(const std::string&)>;

// Observer interface for app state changes
class AppStateObserver {
public:

	virtual void onAppStarted(const std::string& packageName) = 0;
	virtual void onAppStopped(const std::string& packageName) = 0;
	virtual ~AppStateObserver() = default;
};

/**
 * @brief Entry in the app launch stack
 *
 * Tracks a running app instance along with its context, launch ID,
 * and optional result callback for parent app result delivery.
 */
struct AppStackEntry {
	std::shared_ptr<App> app;
	std::unique_ptr<AppContext> context;
	LaunchId launchId = LAUNCH_ID_INVALID;
	ResultCallback resultCallback;
};

class AppManager : public flx::Singleton<AppManager> {
	friend class flx::Singleton<AppManager>;

public:

	void init();
	void registerApp(std::shared_ptr<App> app);
	const std::vector<std::shared_ptr<App>>& getInstalledApps() const;

	// === UI Integration ===
	void setGuiCallbacks(GuiLockCallback lock, GuiUnlockCallback unlock);
	void setWindowCallbacks(WindowOpenCallback open, WindowCloseCallback close);

	// === Intent-based app lifecycle (Phase 2) ===

	/**
	 * Launch an app via an Intent. If targetAppId is set, launches that app directly.
	 * Otherwise, resolves via IntentResolver using mimeType/action.
	 * @return LaunchId for tracking this launch, or LAUNCH_ID_INVALID on failure.
	 */
	LaunchId startApp(const Intent& intent);

	/**
	 * Launch an app via Intent and register a callback to receive its result.
	 * The callback is invoked when the launched app calls finish() or is closed.
	 * @return LaunchId for tracking this launch, or LAUNCH_ID_INVALID on failure.
	 */
	LaunchId startAppForResult(const Intent& intent, ResultCallback callback);

	/**
	 * Finish an app instance and deliver its result to the parent.
	 * Called by the app itself (via AppContext) or by the system when closing.
	 */
	void finishApp(LaunchId id, ResultCode resultCode = ResultCode::Cancelled, const flx::core::Bundle& resultData = {});

	/**
	 * Stop an app by package name.
	 * @param closeUI If true, requests WindowManager to close the window (preventing loops via closeUI=false).
	 */
	bool stopApp(const std::string& packageName, bool closeUI = true);

	/**
	 * Stop the currently active app (top of stack).
	 */
	void stopCurrentApp();

	/**
	 * Get the AppContext for a specific launch instance.
	 */
	AppContext* getContext(LaunchId id) const;

	// === App queries ===

	std::shared_ptr<App> getAppByPackageName(const std::string& packageName);
	std::shared_ptr<App> getCurrentApp() const;
	bool isAppRegistered(const std::string& packageName) const;

	// === App stack queries ===

	/** Get the current app stack depth */
	size_t getStackDepth() const;

	/** Check if an app (by package name) is anywhere in the stack */
	bool isAppInStack(const std::string& packageName) const;

	// === Observer pattern ===

	void addObserver(AppStateObserver* observer);
	void removeObserver(AppStateObserver* observer);

	// === Diagnostics ===

	void performHealthCheck();
	void update();

private:

	AppManager();
	~AppManager() = default;

	// === App stack (Phase 2) ===
	std::vector<AppStackEntry> m_appStack;
	LaunchId m_nextLaunchId = 1;

	// === Registered apps ===
	std::vector<std::shared_ptr<App>> m_apps;
	std::vector<AppStateObserver*> m_observers {};

	void* m_mutex = nullptr;
	void* m_executor = nullptr;

	// Internal helpers
	LaunchId generateLaunchId();
	void notifyAppStarted(const std::string& packageName);
	void notifyAppStopped(const std::string& packageName);
	void publishAppEvent(const char* event, const std::string& appId);

	// Legacy method removed, but internal logic moved to startAppForResult
	// bool startApp(std::shared_ptr<App> app);
};

} // namespace flx::apps
