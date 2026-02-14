#include "AppManager.hpp"
#include "AppManifest.hpp"
#include "AppRegistry.hpp"
#include "EventBus.hpp"
#include "calendar/CalendarApp.hpp"
#include <flx/core/Logger.hpp>
#include "core/services/ServiceRegistry.hpp"
#include "core/tasks/TaskManager.hpp"
#include "core/tasks/gui/GuiTask.hpp"
#include "core/ui/desktop/Desktop.hpp"
#include "esp_system.h"
#include "files/FilesApp.hpp"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "freertos/semphr.h"
#include "image_viewer/ImageViewerApp.hpp"
#include "portmacro.h"
#include "settings/SettingsApp.hpp"
#include "system_info/SystemInfoApp.hpp"
#include "text_editor/TextEditorApp.hpp"
#include "tools/ToolsApp.hpp"
#include <algorithm> // Explicitly include for std::find_if

namespace System::Apps {

// ============================================================
// Static manifest definitions for all built-in apps
// ============================================================

const AppManifest SettingsApp::manifest = {
	.appId = "com.flxos.settings",
	.appName = "Settings",
	.appIcon = LV_SYMBOL_SETTINGS,
	.appVersionName = "1.1.0",
	.appVersionCode = 2,
	.category = AppCategory::System,
	.flags = AppFlags::SingleInstance,
	.location = AppLocation::internal(),
	.description = "System configuration and preferences",
	.sortPriority = 10,
	.capabilities = AppCapability::WiFi | AppCapability::Bluetooth,
	.createApp = []() -> std::shared_ptr<App> { return std::make_shared<SettingsApp>(); }
};

const AppManifest FilesApp::manifest = {
	.appId = "com.flxos.files",
	.appName = "Files",
	.appIcon = LV_SYMBOL_DIRECTORY,
	.appVersionName = "1.0.0",
	.appVersionCode = 1,
	.category = AppCategory::System,
	.flags = AppFlags::None,
	.location = AppLocation::internal(),
	.description = "Browse and manage files on device storage",
	.sortPriority = 20,
	.capabilities = AppCapability::Storage,
	.createApp = []() -> std::shared_ptr<App> { return std::make_shared<FilesApp>(); }
};

const AppManifest SystemInfoApp::manifest = {
	.appId = "com.flxos.systeminfo",
	.appName = "System Info",
	.appIcon = LV_SYMBOL_TINT,
	.appVersionName = "1.0.0",
	.appVersionCode = 1,
	.category = AppCategory::System,
	.flags = AppFlags::SingleInstance,
	.location = AppLocation::internal(),
	.description = "System diagnostics and hardware information",
	.sortPriority = 30,
	.createApp = []() -> std::shared_ptr<App> { return std::make_shared<SystemInfoApp>(); }
};

const AppManifest CalendarApp::manifest = {
	.appId = "com.flxos.calendar",
	.appName = "Calendar",
	.appIcon = LV_SYMBOL_LEFT,
	.appVersionName = "1.0.0",
	.appVersionCode = 1,
	.category = AppCategory::User,
	.flags = AppFlags::None,
	.location = AppLocation::internal(),
	.description = "View calendar and dates",
	.sortPriority = 50,
	.createApp = []() -> std::shared_ptr<App> { return std::make_shared<CalendarApp>(); }
};

const AppManifest TextEditorApp::manifest = {
	.appId = "com.flxos.texteditor",
	.appName = "Text Editor",
	.appIcon = LV_SYMBOL_EDIT,
	.appVersionName = "1.0.0",
	.appVersionCode = 1,
	.category = AppCategory::User,
	.flags = AppFlags::None,
	.location = AppLocation::internal(),
	.description = "Create and edit text files",
	.sortPriority = 40,
	.capabilities = AppCapability::Storage,
	.supportedMimeTypes = {"text/plain", "text/*"},
	.createApp = []() -> std::shared_ptr<App> { return std::make_shared<TextEditorApp>(); }
};

const AppManifest ToolsApp::manifest = {
	.appId = "com.flxos.tools",
	.appName = "Tools",
	.appIcon = LV_SYMBOL_LIST,
	.appVersionName = "1.0.0",
	.appVersionCode = 1,
	.category = AppCategory::Tools,
	.flags = AppFlags::None,
	.location = AppLocation::internal(),
	.description = "Calculator, Stopwatch, Flashlight, Display Tester",
	.sortPriority = 45,
	.capabilities = AppCapability::GPIO,
	.createApp = []() -> std::shared_ptr<App> { return std::make_shared<ToolsApp>(); }
};

const AppManifest ImageViewerApp::manifest = {
	.appId = "com.flxos.imageviewer",
	.appName = "Image Viewer",
	.appIcon = LV_SYMBOL_IMAGE,
	.appVersionName = "1.0.0",
	.appVersionCode = 1,
	.category = AppCategory::System,
	.flags = AppFlags::Hidden,
	.location = AppLocation::internal(),
	.description = "View image files",
	.sortPriority = 100,
	.capabilities = AppCapability::Storage,
	.supportedMimeTypes = {"image/bmp", "image/*"},
	.createApp = []() -> std::shared_ptr<App> { return std::make_shared<ImageViewerApp>(); }
};

// ============================================================

class AppExecutor : public System::Task {
public:

	AppExecutor() : System::Task("app_executor", 8 * 1024, 4) {
		setRestartPolicy(RestartPolicy::RESTART_TASK);
	}

protected:

	void run(void* /*data*/) override {
		setWatchdogTimeout(10000);
		while (true) {
			heartbeat();
			AppManager::getInstance().update();
			vTaskDelay(pdMS_TO_TICKS(16));
		}
	}
};

AppManager::AppManager() : m_mutex(xSemaphoreCreateMutex()) {}

void AppManager::init() {
	Log::info("AppManager", "Initializing AppManager...");

	// Register all built-in app manifests to the AppRegistry
	auto& registry = AppRegistry::getInstance();
	registry.addApp(SettingsApp::manifest);
	registry.addApp(FilesApp::manifest);
	registry.addApp(SystemInfoApp::manifest);
	registry.addApp(CalendarApp::manifest);
	registry.addApp(TextEditorApp::manifest);
	registry.addApp(ToolsApp::manifest);
	registry.addApp(ImageViewerApp::manifest);

	// Instantiate apps from registry
	for (const auto& manifest: registry.getAll()) {
		if (manifest.createApp) {
			registerApp(manifest.createApp());
		}
	}

	if (!m_executor) {
		Log::info("AppManager", "Starting AppExecutor task...");
		m_executor = new AppExecutor();
		static_cast<AppExecutor*>(m_executor)->start();
	}

	Log::info("AppManager", "App stack initialized.");
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

// ============================================================
// Intent-based app lifecycle (Phase 2)
// ============================================================

LaunchId AppManager::startApp(const Intent& intent) {
	return startAppForResult(intent, nullptr);
}

LaunchId AppManager::startAppForResult(const Intent& intent, ResultCallback callback) {
	// Resolve intent to an app
	auto manifestOpt = IntentResolver::resolve(intent);
	if (!manifestOpt) {
		Log::error("AppManager", "No app found to handle intent (action=%s, mime=%s, target=%s)", intent.action.c_str(), intent.mimeType.c_str(), intent.targetAppId.c_str());
		return LAUNCH_ID_INVALID;
	}

	const auto& manifest = *manifestOpt;

	// === Pre-launch validation ===

	// Check required services are running
	for (const auto& svcId: manifest.requiredServices) {
		auto state = Services::ServiceRegistry::getInstance().getServiceState(svcId);
		if (state != Services::ServiceState::Started) {
			Log::error("AppManager", "App '%s' requires service '%s' which is not running", manifest.appId.c_str(), svcId.c_str());
			return LAUNCH_ID_INVALID;
		}
	}

	// Check minimum heap requirement
	if (manifest.minHeapKb > 0) {
		uint32_t freeKb = esp_get_free_heap_size() / 1024;
		if (freeKb < manifest.minHeapKb) {
			Log::error("AppManager", "Not enough heap for '%s' (need %u KB, have %lu KB)", manifest.appId.c_str(), manifest.minHeapKb, (unsigned long)freeKb);
			return LAUNCH_ID_INVALID;
		}
	}

	auto app = getAppByPackageName(manifest.appId);
	if (!app) {
		Log::error("AppManager", "App '%s' resolved but not registered", manifest.appId.c_str());
		return LAUNCH_ID_INVALID;
	}

	LaunchId launchId = LAUNCH_ID_INVALID;

	Log::info("AppManager", "startAppForResult: Acquiring mutex for %s", manifest.appId.c_str());
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);
	Log::info("AppManager", "startAppForResult: Mutex acquired");

	// Check if already in stack
	auto it = std::find_if(m_appStack.begin(), m_appStack.end(), [&](const AppStackEntry& e) { return e.app && e.app->getPackageName() == manifest.appId; });

	if (it != m_appStack.end()) {
		// Found existing instance
		Log::info("AppManager", "Bringing existing app to front: %s", manifest.appId.c_str());

		// Move to back (top of stack)
		AppStackEntry entry = std::move(*it); // Move out
		m_appStack.erase(it); // Remove from old spot

		// Update context with new intent
		if (entry.context) {
			// In a real Android-like system we'd call onNewIntent
			// For now, let's just update the intent in the context if possible or ignore
		}

		// Pause current top if specific
		if (!m_appStack.empty()) {
			auto current = m_appStack.back().app;
			if (current) {
				GuiTask::lock();
				current->onPause();
				GuiTask::unlock();
			}
		}

		launchId = entry.launchId;
		// Push to top
		m_appStack.push_back(std::move(entry));

		xSemaphoreGive((SemaphoreHandle_t)m_mutex);

		// Resume
		GuiTask::lock();
		if (app) {
			app->setActive(true);
			app->onResume();
		}
		Desktop::getInstance().openApp(manifest.appId);
		GuiTask::unlock();

		return launchId;
	}

	launchId = generateLaunchId();

	// 1. Pause current app if exists
	if (!m_appStack.empty()) {
		auto current = m_appStack.back().app;
		if (current) {
			GuiTask::lock();
			current->onPause();
			GuiTask::unlock();
		}
	}

	// 2. Create context
	auto ctx = std::make_unique<AppContext>(&manifest, intent, launchId);
	if (callback) {
		ctx->setResultCallback(callback);
	}

	// 3. Push to stack
	AppStackEntry entry;
	entry.app = app;
	entry.launchId = launchId;
	entry.resultCallback = callback;
	entry.context = std::move(ctx);

	app->setContext(entry.context.get());
	m_appStack.push_back(std::move(entry));

	xSemaphoreGive((SemaphoreHandle_t)m_mutex);

	// 4. Start lifecycle
	GuiTask::lock();
	if (!app->onStart()) {
		Log::error("AppManager", "Failed to start app: %s", manifest.appId.c_str());
		stopApp(manifest.appId, true); // Cleanup
		GuiTask::unlock();
		return LAUNCH_ID_INVALID;
	}
	app->setActive(true);
	app->onResume();
	GuiTask::unlock();

	Log::info("AppManager", "Started app: %s (launchId=%lu, action=%s)", manifest.appId.c_str(), (unsigned long)launchId, intent.action.c_str());

	notifyAppStarted(manifest.appId);
	publishAppEvent(Events::APP_STARTED, manifest.appId);

	Log::info("AppManager", "startAppForResult: Requesting Desktop openApp");

	// 5. Open UI
	// Desktop will call WindowManager::openApp -> WindowManager creates window -> Calls app->createUI
	// WindowManager should NOT call AppManager::startApp() anymore.
	GuiTask::lock();
	Desktop::getInstance().openApp(manifest.appId);
	GuiTask::unlock();

	Log::info("AppManager", "startAppForResult: Start complete");

	return launchId;
}

void AppManager::finishApp(LaunchId id, ResultCode resultCode, const Bundle& resultData) {
	if (id == LAUNCH_ID_INVALID) return;

	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);

	// Find in stack
	auto it = m_appStack.begin();
	while (it != m_appStack.end()) {
		if (it->launchId == id) {
			break;
		}
		++it;
	}

	if (it == m_appStack.end()) {
		xSemaphoreGive((SemaphoreHandle_t)m_mutex);
		Log::error("AppManager", "finishApp: LaunchId %lu not found", (unsigned long)id);
		return;
	}

	auto app = it->app;
	auto resultCb = it->resultCallback;
	bool wasActive = (it == m_appStack.end() - 1); // Is top of stack

	// Set result in context
	if (it->context) {
		it->context->setResult(resultCode, resultData);
	}

	std::string pkg = app->getPackageName();

	// If active, stop lifecycle
	if (wasActive && app) {
		GuiTask::lock();
		app->onPause();
		app->onStop();
		app->setActive(false);
		GuiTask::unlock();
	}

	if (app) app->setContext(nullptr);

	m_appStack.erase(it);

	// Delivery logic
	LaunchId parentId = LAUNCH_ID_INVALID;
	std::shared_ptr<App> parentApp = nullptr;

	if (!m_appStack.empty()) {
		parentApp = m_appStack.back().app;
		// If we wanted to track parent ID, we could.
	}

	xSemaphoreGive((SemaphoreHandle_t)m_mutex);

	// Deliver to callback
	if (resultCb) {
		resultCb(resultCode, resultData);
	}

	// Deliver to parent app if stack not empty and was active
	if (wasActive && parentApp) {
		GuiTask::lock();
		parentApp->onResult(resultCode, resultData);
		parentApp->onResume();
		GuiTask::unlock();
	}

	notifyAppStopped(pkg);
	publishAppEvent(Events::APP_STOPPED, pkg);

	// Close UI
	GuiTask::lock();
	Desktop::getInstance().closeApp(pkg);
	GuiTask::unlock();
}

bool AppManager::stopApp(const std::string& packageName, bool closeUI) {
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);

	// Find in stack (could be multiple instances? For now assume finding last for that pkg)
	// Iterate backwards to find latest
	auto it = m_appStack.rbegin();
	while (it != m_appStack.rend()) {
		if (it->app && it->app->getPackageName() == packageName) {
			break;
		}
		++it;
	}

	if (it == m_appStack.rend()) {
		xSemaphoreGive((SemaphoreHandle_t)m_mutex);
		return false; // Not running
	}

	// Convert reverse iterator to forward iterator to erase
	auto forward_it = std::next(it).base();

	auto app = forward_it->app;
	bool wasActive = (forward_it == m_appStack.end() - 1);

	LaunchId id = forward_it->launchId; // Capture ID

	// Lifecycle
	GuiTask::lock();
	if (app->isActive()) {
		app->onPause();
		app->onStop();
		app->setActive(false);
	}
	GuiTask::unlock();

	if (app) app->setContext(nullptr);

	m_appStack.erase(forward_it);

	// Resume previous if we removed the top
	std::shared_ptr<App> newTop = nullptr;
	if (wasActive && !m_appStack.empty()) {
		newTop = m_appStack.back().app;
	}

	xSemaphoreGive((SemaphoreHandle_t)m_mutex);

	notifyAppStopped(packageName);
	publishAppEvent(Events::APP_STOPPED, packageName);

	if (wasActive && newTop) {
		GuiTask::lock();
		newTop->onResume();
		GuiTask::unlock();
	}

	if (closeUI) {
		GuiTask::lock();
		Desktop::getInstance().closeApp(packageName);
		GuiTask::unlock();
	}

	return true;
}

void AppManager::stopCurrentApp() {
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);
	if (m_appStack.empty()) {
		xSemaphoreGive((SemaphoreHandle_t)m_mutex);
		return;
	}
	auto app = m_appStack.back().app;
	xSemaphoreGive((SemaphoreHandle_t)m_mutex);

	if (app) {
		stopApp(app->getPackageName());
	}
}

AppContext* AppManager::getContext(LaunchId id) const {
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);
	for (const auto& entry: m_appStack) {
		if (entry.launchId == id) {
			auto* ctx = entry.context.get();
			xSemaphoreGive((SemaphoreHandle_t)m_mutex);
			return ctx;
		}
	}
	xSemaphoreGive((SemaphoreHandle_t)m_mutex);
	return nullptr;
}

// ============================================================
// App stack queries
// ============================================================

size_t AppManager::getStackDepth() const {
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);
	size_t depth = m_appStack.size();
	xSemaphoreGive((SemaphoreHandle_t)m_mutex);
	return depth;
}

bool AppManager::isAppInStack(const std::string& packageName) const {
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);
	for (const auto& entry: m_appStack) {
		if (entry.app && entry.app->getPackageName() == packageName) {
			xSemaphoreGive((SemaphoreHandle_t)m_mutex);
			return true;
		}
	}
	xSemaphoreGive((SemaphoreHandle_t)m_mutex);
	return false;
}

// ============================================================
// Helper methods
// ============================================================

LaunchId AppManager::generateLaunchId() {
	// Mutex should be held by caller
	LaunchId id = m_nextLaunchId++;
	if (m_nextLaunchId == LAUNCH_ID_INVALID) m_nextLaunchId = 1; // Wrap around, skip 0
	return id;
}

void AppManager::update() {
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);
	std::shared_ptr<App> activeApp = nullptr;
	if (!m_appStack.empty()) {
		activeApp = m_appStack.back().app;
	}
	xSemaphoreGive((SemaphoreHandle_t)m_mutex);

	if (activeApp) {
		GuiTask::lock();
		activeApp->update();
		GuiTask::unlock();
	}
}

const std::vector<std::shared_ptr<App>>& AppManager::getInstalledApps() const {
	return m_apps;
}

std::shared_ptr<App> AppManager::getCurrentApp() const {
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);
	std::shared_ptr<App> app = nullptr;
	if (!m_appStack.empty()) {
		app = m_appStack.back().app;
	}
	xSemaphoreGive((SemaphoreHandle_t)m_mutex);
	return app;
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
		observer->onAppStarted(packageName);
	}
}

void AppManager::notifyAppStopped(const std::string& packageName) {
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);
	auto observers = m_observers; // Copy to avoid holding lock during callbacks
	xSemaphoreGive((SemaphoreHandle_t)m_mutex);

	for (auto* observer: observers) {
		observer->onAppStopped(packageName);
	}
}

void AppManager::publishAppEvent(const char* event, const std::string& appId) {
	Bundle data;
	data.putString("appId", appId);
	EventBus::getInstance().publish(event, data);
}

void AppManager::performHealthCheck() {
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);

	size_t stackSize = m_appStack.size();
	std::string topApp = stackSize > 0 ? m_appStack.back().app->getPackageName() : "None";

	Log::info("AppManager", "Health: %d apps registered, %zu in stack, Top: %s", (int)m_apps.size(), stackSize, topApp.c_str());

	xSemaphoreGive((SemaphoreHandle_t)m_mutex);
}

} // namespace System::Apps
