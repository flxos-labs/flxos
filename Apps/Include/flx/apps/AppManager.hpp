#pragma once

#include <cstdint>
#include <flx/core/Bundle.hpp>
#include <flx/core/Singleton.hpp>
#include <flx/kernel/TaskManager.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "App.hpp"
#include "AppContext.hpp"
#include "Intent.hpp"

namespace flx::apps {

class AppExecutor;

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
	int64_t activeSinceUs = 0;
};

struct AppLaunchStats {
	uint32_t launchCount = 0;
	int64_t lastStartTimeUs = 0;
	int32_t heapDeltaBytes = 0;
	uint32_t lastActiveTimeMs = 0;
	uint32_t totalActiveTimeMs = 0;
};

static constexpr int MAX_CRASHES_BEFORE_BLOCK = 3;
static constexpr int CRASH_WINDOW_SECONDS = 60;

/**
 * @brief Tracks crash history for a single app (crash recovery, 2.5)
 */
struct AppCrashRecord {
	std::string appId;
	uint32_t timestamps[MAX_CRASHES_BEFORE_BLOCK] = {}; ///< Ring buffer of last crash timestamps (seconds)
	uint8_t index = 0; ///< Next write position in ring buffer
	uint32_t crashCount = 0; ///< Total crash count
	std::string lastError; ///< Last crash reason
};

class AppManager : public flx::Singleton<AppManager> {
	friend class flx::Singleton<AppManager>;
	friend class AppExecutor;

public:

	void init();
	void registerApp(std::shared_ptr<App> app);

	/**
	 * @brief Returns the current live app instances managed by AppManager.
	 *
	 * This reflects running/cached app objects, not the manifest-level installed
	 * application catalog.
	 */
	std::vector<std::shared_ptr<App>> getLiveApps() const;

	/**
	 * @brief Deprecated compatibility wrapper for getLiveApps().
	 *
	 * Despite the legacy name, this returns live app instances rather than the
	 * installed manifest catalog.
	 */
	[[deprecated("Use getLiveApps(); getInstalledApps() returns live app instances, not the installed app catalog.")]]
	std::vector<std::shared_ptr<App>> getInstalledApps() const {
		return getLiveApps();
	}
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
	AppLaunchStats getAppStats(const std::string& packageName) const;

	// === App stack queries ===

	/** Get the current app stack depth */
	size_t getStackDepth() const;

	/** Check if an app (by package name) is anywhere in the stack */
	bool isAppInStack(const std::string& packageName) const;

	// === Observer pattern ===

	void addObserver(AppStateObserver* observer);
	void removeObserver(AppStateObserver* observer);

	// === Diagnostics ===

	void dumpAppStates() const;
	void performHealthCheck();
	void update();

	// === Crash recovery (2.5) ===

	/**
	 * Report that an app has crashed.
	 * Increments crash count and may block the app if threshold is exceeded.
	 */
	void reportAppCrash(const std::string& appId, const std::string& reason);

	/**
	 * Check if an app is blocked due to repeated crashes.
	 */
	bool isAppBlocked(const std::string& appId) const;

	/**
	 * Clear crash history for an app, allowing it to be launched again.
	 */
	void clearCrashHistory(const std::string& appId);

private:

	struct AppCommand;

	AppManager();
	~AppManager() = default;

	// === App stack (Phase 2) ===
	std::vector<AppStackEntry> m_appStack;
	LaunchId m_nextLaunchId = 1;

	// === Live app instances ===
	std::vector<std::shared_ptr<App>> m_liveApps;
	std::unordered_map<std::string, AppLaunchStats> m_appStats;
	std::unordered_map<std::string, AppCrashRecord> m_crashRecords;
	std::vector<AppStateObserver*> m_observers {};

	void* m_mutex = nullptr;
	void* m_executor = nullptr;
	void* m_dispatcherQueue = nullptr;
	uint32_t m_memoryCriticalSubscription = 0;

	// Internal helpers
	bool dispatchAndWait(AppCommand& cmd);
	void processQueuedCommands();
	void processCommand(AppCommand& cmd);
	bool isExecutorThread() const;
	std::shared_ptr<App> findLiveAppLocked(const std::string& packageName) const;
	void removeLiveAppLocked(const std::string& packageName);

	LaunchId startAppForResultImpl(const Intent& intent, ResultCallback callback);
	void finishAppImpl(LaunchId id, ResultCode resultCode, const flx::core::Bundle& resultData);
	bool stopAppImpl(const std::string& packageName, bool closeUI);

	LaunchId generateLaunchId();
	void notifyAppStarted(const std::string& packageName);
	void notifyAppStopped(const std::string& packageName);
	void publishAppEvent(const char* event, const std::string& appId);
	void recordLaunchMetrics(const std::string& packageName, int64_t startTimeUs, int32_t heapDeltaBytes);
	void recordActiveTime(AppStackEntry& entry, int64_t nowUs);
	void markTopAppActive(int64_t nowUs);
	bool evictOldestPausedApp(const char* reason);

	// Legacy method removed, but internal logic moved to startAppForResult
	// bool startApp(std::shared_ptr<App> app);
};

} // namespace flx::apps
