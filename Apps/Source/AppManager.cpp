#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "portmacro.h"
#include <algorithm> // Explicitly include for std::find_if
#include <flx/apps/AppManager.hpp>
#include <flx/apps/AppManifest.hpp>
#include <flx/apps/AppRegistry.hpp>
#include <flx/core/BootTimeline.hpp>
#include <flx/core/EventBus.hpp>
#include <flx/core/GuiLock.hpp>
#include <flx/core/Logger.hpp>
#include <flx/hal/HardwareCapabilities.hpp>
#include <flx/kernel/TaskManager.hpp>
#include <flx/services/ServiceRegistry.hpp>
#include <limits>
#include <utility>

namespace flx::apps {

namespace {

uint32_t durationUsToMs(int64_t durationUs) {
	if (durationUs <= 0) {
		return 0;
	}

	const int64_t durationMs = durationUs / 1000;
	if (durationMs >= static_cast<int64_t>(std::numeric_limits<uint32_t>::max())) {
		return std::numeric_limits<uint32_t>::max();
	}

	return static_cast<uint32_t>(durationMs);
}

uint32_t saturatingAddMs(uint32_t totalMs, uint32_t deltaMs) {
	if (deltaMs > std::numeric_limits<uint32_t>::max() - totalMs) {
		return std::numeric_limits<uint32_t>::max();
	}

	return totalMs + deltaMs;
}

} // namespace

struct AppManager::AppCommand {
	enum class Type {
		Start,
		Stop,
		Finish,
	};

	Type type = Type::Start;
	Intent intent {};
	ResultCallback callback {};
	std::string packageName {};
	LaunchId launchId = LAUNCH_ID_INVALID;
	ResultCode resultCode = ResultCode::Cancelled;
	flx::core::Bundle resultData {};
	bool closeUI = true;
	bool stopResult = false;
	SemaphoreHandle_t completion = nullptr;
};

class AppExecutor : public flx::kernel::Task {
public:

	AppExecutor() : flx::kernel::Task("app_executor", 8 * 1024, 4) {
		setRestartPolicy(RestartPolicy::RESTART_TASK);
	}

protected:

	void run(void* /*data*/) override {
		setWatchdogTimeout(10000);
		while (true) {
			heartbeat();
			AppManager::getInstance().processQueuedCommands();
			AppManager::getInstance().update();
		}
	}
};

AppManager::AppManager()
	: m_mutex(xSemaphoreCreateMutex()),
	  m_dispatcherQueue(xQueueCreate(16, sizeof(AppCommand*))) {}

// Callback storage
static GuiLockCallback s_guiLock;
static GuiUnlockCallback s_guiUnlock;
static WindowOpenCallback s_windowOpen;
static WindowCloseCallback s_windowClose;

void AppManager::setGuiCallbacks(GuiLockCallback lock, GuiUnlockCallback unlock) {
	s_guiLock = lock;
	s_guiUnlock = unlock;
}

void AppManager::setWindowCallbacks(WindowOpenCallback open, WindowCloseCallback close) {
	s_windowOpen = open;
	s_windowClose = close;
}

static void lockGui() {
	if (s_guiLock) s_guiLock();
}

static void unlockGui() {
	if (s_guiUnlock) s_guiUnlock();
}

void AppManager::init() {
	Log::info("AppManager", "Initializing AppManager...");

	// Note: Built-in apps are now registered externally (e.g. in SystemManager or Main)

	// Instantiate apps from registry
	auto& registry = AppRegistry::getInstance();
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
	if (isExecutorThread() || !m_dispatcherQueue || !m_executor) {
		return startAppForResultImpl(intent, std::move(callback));
	}

	AppCommand cmd;
	cmd.type = AppCommand::Type::Start;
	cmd.intent = intent;
	cmd.callback = std::move(callback);
	cmd.completion = xSemaphoreCreateBinary();
	if (!cmd.completion) {
		Log::error("AppManager", "Failed to allocate start command completion semaphore");
		return LAUNCH_ID_INVALID;
	}

	bool dispatched = dispatchAndWait(cmd);
	vSemaphoreDelete(cmd.completion);
	return dispatched ? cmd.launchId : LAUNCH_ID_INVALID;
}

LaunchId AppManager::startAppForResultImpl(const Intent& intent, ResultCallback callback) {
	// Resolve intent to an app
	auto manifestOpt = IntentResolver::resolve(intent);
	if (!manifestOpt) {
		Log::error("AppManager", "No app found to handle intent (action=%s, mime=%s, target=%s)", intent.action.c_str(), intent.mimeType.c_str(), intent.targetAppId.c_str());
		return LAUNCH_ID_INVALID;
	}

	const auto& manifest = *manifestOpt;

	// === Pre-launch validation ===

	// Check if app is blocked due to repeated crashes (2.5)
	if (isAppBlocked(manifest.appId)) {
		Log::error("AppManager", "App '%s' is blocked due to repeated crashes", manifest.appId.c_str());
		return LAUNCH_ID_INVALID;
	}

	// Check required services are running
	for (const auto& svcId: manifest.requiredServices) {
		auto state = flx::services::ServiceRegistry::getInstance().getServiceState(svcId);
		if (state != flx::services::ServiceState::Started) {
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

	// Hardware capabilities validation
	if (manifest.capabilities != AppCapability::None) {
		auto hwCaps = flx::hal::getCapabilities();

		if (hasCapability(manifest.capabilities, AppCapability::WiFi) && !hwCaps.hasWifi()) {
			Log::error("AppManager", "App '%s' requires WiFi (not available)", manifest.appId.c_str());
			return LAUNCH_ID_INVALID;
		}
		if (hasCapability(manifest.capabilities, AppCapability::Bluetooth) && !hwCaps.hasBluetooth()) {
			Log::error("AppManager", "App '%s' requires Bluetooth (not available)", manifest.appId.c_str());
			return LAUNCH_ID_INVALID;
		}
		if (hasCapability(manifest.capabilities, AppCapability::I2C) && !hwCaps.hasI2C()) {
			Log::error("AppManager", "App '%s' requires I2C (not available)", manifest.appId.c_str());
			return LAUNCH_ID_INVALID;
		}
		if (hasCapability(manifest.capabilities, AppCapability::SPI) && !hwCaps.hasSpi()) {
			Log::error("AppManager", "App '%s' requires SPI (not available)", manifest.appId.c_str());
			return LAUNCH_ID_INVALID;
		}
		if (hasCapability(manifest.capabilities, AppCapability::UART) && !hwCaps.hasUart()) {
			Log::error("AppManager", "App '%s' requires UART (not available)", manifest.appId.c_str());
			return LAUNCH_ID_INVALID;
		}
		if (hasCapability(manifest.capabilities, AppCapability::GPIO) && !hwCaps.hasGpio()) {
			Log::error("AppManager", "App '%s' requires GPIO (not available)", manifest.appId.c_str());
			return LAUNCH_ID_INVALID;
		}
		if (hasCapability(manifest.capabilities, AppCapability::Storage) && !hwCaps.hasSdCard()) {
			Log::error("AppManager", "App '%s' requires Storage (not available)", manifest.appId.c_str());
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
		int64_t nowUs = esp_timer_get_time();

		// Found existing instance
		Log::info("AppManager", "Bringing existing app to front: %s", manifest.appId.c_str());

		if (!m_appStack.empty()) {
			recordActiveTime(m_appStack.back(), nowUs);
			auto current = m_appStack.back().app;
			if (current) {
				lockGui();
				current->onPause();
				unlockGui();
			}
		}

		// Move to back (top of stack)
		AppStackEntry entry = std::move(*it); // Move out
		m_appStack.erase(it); // Remove from old spot

		// Update context with new intent
		if (entry.context) {
			entry.context->setIntent(intent);
		}

		launchId = entry.launchId;
		// Push to top
		m_appStack.push_back(std::move(entry));

		xSemaphoreGive((SemaphoreHandle_t)m_mutex);

		// Resume
		lockGui();
		if (app) {
			app->onNewIntent(intent);
			app->setActive(true);
			app->onResume();
		}
		markTopAppActive(esp_timer_get_time());
		if (s_windowOpen) s_windowOpen(manifest.appId);
		unlockGui();

		return launchId;
	}

	launchId = generateLaunchId();

	// 1. Pause current app if exists
	if (!m_appStack.empty()) {
		recordActiveTime(m_appStack.back(), esp_timer_get_time());
		auto current = m_appStack.back().app;
		if (current) {
			lockGui();
			current->onPause();
			unlockGui();
		}
	}

	// 2. Create context
	auto ctx = std::make_unique<AppContext>(&manifest, intent, launchId);
	auto paths = ctx->getPaths();
	if (!paths.ensureDirectories()) {
		xSemaphoreGive((SemaphoreHandle_t)m_mutex);
		Log::error("AppManager", "Failed to prepare app storage for '%s'", manifest.appId.c_str());
		return LAUNCH_ID_INVALID;
	}
	if (callback) {
		ctx->setResultCallback(callback);
	}

	// 3. Push to stack
	AppStackEntry entry;
	entry.app = app;
	entry.launchId = launchId;
	entry.resultCallback = callback;
	auto prefs = ctx->getPreferences();
	entry.context = std::move(ctx);

	app->setContext(entry.context.get());
	m_appStack.push_back(std::move(entry));

	xSemaphoreGive((SemaphoreHandle_t)m_mutex);

	// 4. Start lifecycle
	uint32_t heapBeforeStart = esp_get_free_heap_size();
	int64_t startBeginUs = esp_timer_get_time();
	lockGui();
	if (!app->onStart()) {
		Log::error("AppManager", "Failed to start app: %s", manifest.appId.c_str());
		stopAppImpl(manifest.appId, true); // Cleanup
		unlockGui();
		return LAUNCH_ID_INVALID;
	}

	// Restore state (after onStart)
	std::vector<uint8_t> savedData;
	if (prefs.optBlob("_saved_state_", savedData)) {
		int64_t savedTime = prefs.getInt64Or("_saved_state_time_", 0);
		// 24 hours TTL
		if (esp_timer_get_time() - savedTime < 24LL * 60 * 60 * 1000000LL) {
			flx::core::Bundle state = flx::core::Bundle::deserialize(savedData);
			app->onRestoreState(state);
		}
		prefs.erase("_saved_state_");
		prefs.erase("_saved_state_time_");
	}

	int64_t lastStartTimeUs = esp_timer_get_time() - startBeginUs;
	int32_t heapDeltaBytes = static_cast<int32_t>(esp_get_free_heap_size()) - static_cast<int32_t>(heapBeforeStart);
	app->setActive(true);
	app->onResume();
	unlockGui();

	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);
	recordLaunchMetrics(manifest.appId, lastStartTimeUs, heapDeltaBytes);
	if (!m_appStack.empty() && m_appStack.back().launchId == launchId) {
		m_appStack.back().activeSinceUs = esp_timer_get_time();
	}
	xSemaphoreGive((SemaphoreHandle_t)m_mutex);

	Log::info("AppManager", "Started app: %s (launchId=%lu, action=%s)", manifest.appId.c_str(), (unsigned long)launchId, intent.action.c_str());

	notifyAppStarted(manifest.appId);
	publishAppEvent(flx::core::Events::APP_STARTED, manifest.appId);
	flx::core::BootTimeline::getInstance().record(
		"app:" + manifest.appId, "started",
		heapDeltaBytes, lastStartTimeUs);

	Log::info("AppManager", "startAppForResult: Requesting Desktop openApp");

	// 5. Open UI
	lockGui();
	if (s_windowOpen) s_windowOpen(manifest.appId);
	unlockGui();

	Log::info("AppManager", "startAppForResult: Start complete");

	return launchId;
}

void AppManager::finishApp(LaunchId id, ResultCode resultCode, const flx::core::Bundle& resultData) {
	if (isExecutorThread() || !m_dispatcherQueue || !m_executor) {
		finishAppImpl(id, resultCode, resultData);
		return;
	}

	AppCommand cmd;
	cmd.type = AppCommand::Type::Finish;
	cmd.launchId = id;
	cmd.resultCode = resultCode;
	cmd.resultData = resultData;
	cmd.completion = xSemaphoreCreateBinary();
	if (!cmd.completion) {
		Log::error("AppManager", "Failed to allocate finish command completion semaphore");
		return;
	}

	dispatchAndWait(cmd);
	vSemaphoreDelete(cmd.completion);
}

void AppManager::finishAppImpl(LaunchId id, ResultCode resultCode, const flx::core::Bundle& resultData) {
	if (id == LAUNCH_ID_INVALID) return;

	uint32_t heapBefore = esp_get_free_heap_size();

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
	std::weak_ptr<App> appWeak = app;
	auto resultCb = it->resultCallback;
	bool wasActive = (it == m_appStack.end() - 1); // Is top of stack

	// Set result in context
	if (it->context) {
		it->context->setResult(resultCode, resultData);
	}

	std::string pkg = app->getPackageName();

	// Clean up any saved state when finishing
	if (it->context) {
		auto prefs = it->context->getPreferences();
		prefs.erase("_saved_state_");
		prefs.erase("_saved_state_time_");
	}

	// If active, stop lifecycle
	if (wasActive && app) {
		recordActiveTime(*it, esp_timer_get_time());
		lockGui();
		app->onPause();
		app->onStop();
		app->setActive(false);
		unlockGui();
	}

	if (app) app->setContext(nullptr);

	m_appStack.erase(it);
	app.reset();

	// Delivery logic

	std::shared_ptr<App> parentApp = nullptr;

	if (!m_appStack.empty()) {
		parentApp = m_appStack.back().app;
		// If we wanted to track parent ID, we could.
	}

	xSemaphoreGive((SemaphoreHandle_t)m_mutex);

	uint32_t heapAfter = esp_get_free_heap_size();
	int32_t leakedBytes = static_cast<int32_t>(heapBefore) - static_cast<int32_t>(heapAfter);
	if (leakedBytes > 1024) {
		Log::warn("AppManager", "Possible leak: %s retained %ld bytes after finish", pkg.c_str(), (long)leakedBytes);
	}
	long externalRefs = static_cast<long>(appWeak.use_count());
	if (externalRefs > 0) {
		Log::warn("AppManager", "Ref leak: %s still has %ld external refs after finish", pkg.c_str(), externalRefs);
	}

	// Deliver to callback
	if (resultCb) {
		resultCb(resultCode, resultData);
	}

	// Deliver to parent app if stack not empty and was active
	if (wasActive && parentApp) {
		lockGui();
		parentApp->onResult(resultCode, resultData);
		parentApp->onResume();
		unlockGui();
		markTopAppActive(esp_timer_get_time());
	}

	notifyAppStopped(pkg);
	publishAppEvent(flx::core::Events::APP_STOPPED, pkg);
	flx::core::BootTimeline::getInstance().record("app:" + pkg, "stopped");

	// Close UI
	lockGui();
	if (s_windowClose) s_windowClose(pkg);
	unlockGui();
}

bool AppManager::stopApp(const std::string& packageName, bool closeUI) {
	if (isExecutorThread() || !m_dispatcherQueue || !m_executor) {
		return stopAppImpl(packageName, closeUI);
	}

	AppCommand cmd;
	cmd.type = AppCommand::Type::Stop;
	cmd.packageName = packageName;
	cmd.closeUI = closeUI;
	cmd.completion = xSemaphoreCreateBinary();
	if (!cmd.completion) {
		Log::error("AppManager", "Failed to allocate stop command completion semaphore");
		return false;
	}

	bool dispatched = dispatchAndWait(cmd);
	bool result = dispatched && cmd.stopResult;
	vSemaphoreDelete(cmd.completion);
	return result;
}

bool AppManager::stopAppImpl(const std::string& packageName, bool closeUI) {
	uint32_t heapBefore = esp_get_free_heap_size();

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
	std::weak_ptr<App> appWeak = app;
	bool wasActive = (forward_it == m_appStack.end() - 1);

	// Lifecycle
	if (wasActive) {
		recordActiveTime(*forward_it, esp_timer_get_time());
	}
	lockGui();
	if (app->isActive()) {
		// Save state before stopping
		auto prefs = forward_it->context ? forward_it->context->getPreferences() : flx::core::Preferences {"app." + packageName};
		flx::core::Bundle savedState = app->onSaveState();
		if (!savedState.empty()) {
			prefs.putBlob("_saved_state_", savedState.serialize());
			prefs.putInt64("_saved_state_time_", esp_timer_get_time());
		} else {
			prefs.erase("_saved_state_");
			prefs.erase("_saved_state_time_");
		}

		app->onPause();
		app->onStop();
		app->setActive(false);
	}
	unlockGui();

	if (app) app->setContext(nullptr);

	m_appStack.erase(forward_it);
	app.reset();

	// Resume previous if we removed the top
	std::shared_ptr<App> newTop = nullptr;
	if (wasActive && !m_appStack.empty()) {
		newTop = m_appStack.back().app;
	}

	xSemaphoreGive((SemaphoreHandle_t)m_mutex);

	uint32_t heapAfter = esp_get_free_heap_size();
	int32_t leakedBytes = static_cast<int32_t>(heapBefore) - static_cast<int32_t>(heapAfter);
	if (leakedBytes > 1024) {
		Log::warn("AppManager", "Possible leak: %s retained %ld bytes after stop", packageName.c_str(), (long)leakedBytes);
	}
	long externalRefs = static_cast<long>(appWeak.use_count());
	if (externalRefs > 0) {
		Log::warn("AppManager", "Ref leak: %s still has %ld external refs after stop", packageName.c_str(), externalRefs);
	}

	notifyAppStopped(packageName);
	publishAppEvent(flx::core::Events::APP_STOPPED, packageName);
	flx::core::BootTimeline::getInstance().record("app:" + packageName, "stopped");

	if (wasActive && newTop) {
		lockGui();
		newTop->onResume();
		unlockGui();
		markTopAppActive(esp_timer_get_time());
	}

	if (closeUI) {
		lockGui();
		if (s_windowClose) s_windowClose(packageName);
		unlockGui();
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

bool AppManager::dispatchAndWait(AppCommand& cmd) {
	auto queue = static_cast<QueueHandle_t>(m_dispatcherQueue);
	if (!queue || !m_executor) {
		processCommand(cmd);
		return true;
	}

	AppCommand* cmdPtr = &cmd;
	if (xQueueSend(queue, &cmdPtr, portMAX_DELAY) != pdPASS) {
		Log::error("AppManager", "Failed to enqueue app lifecycle command");
		return false;
	}

	UBaseType_t releasedGuiDepth = flx::core::GuiLock::releaseAllForCurrentTask();
	BaseType_t waitResult = xSemaphoreTake(cmd.completion, portMAX_DELAY);
	flx::core::GuiLock::reacquireForCurrentTask(releasedGuiDepth);

	if (waitResult != pdTRUE) {
		Log::error("AppManager", "Timed out waiting for app lifecycle command");
		return false;
	}

	return true;
}

void AppManager::processQueuedCommands() {
	auto queue = static_cast<QueueHandle_t>(m_dispatcherQueue);
	if (!queue) {
		vTaskDelay(pdMS_TO_TICKS(16));
		return;
	}

	AppCommand* cmd = nullptr;
	if (xQueueReceive(queue, &cmd, pdMS_TO_TICKS(16)) != pdPASS) {
		return;
	}

	do {
		if (cmd) {
			processCommand(*cmd);
			if (cmd->completion) {
				xSemaphoreGive(cmd->completion);
			}
		}
		cmd = nullptr;
	} while (xQueueReceive(queue, &cmd, 0) == pdPASS);
}

void AppManager::processCommand(AppCommand& cmd) {
	switch (cmd.type) {
		case AppCommand::Type::Start:
			cmd.launchId = startAppForResultImpl(cmd.intent, std::move(cmd.callback));
			break;
		case AppCommand::Type::Stop:
			cmd.stopResult = stopAppImpl(cmd.packageName, cmd.closeUI);
			break;
		case AppCommand::Type::Finish:
			finishAppImpl(cmd.launchId, cmd.resultCode, cmd.resultData);
			break;
	}
}

bool AppManager::isExecutorThread() const {
	auto* executor = static_cast<AppExecutor*>(m_executor);
	if (!executor) {
		return false;
	}

	TaskHandle_t executorHandle = executor->getHandle();
	return executorHandle != nullptr && executorHandle == xTaskGetCurrentTaskHandle();
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

AppLaunchStats AppManager::getAppStats(const std::string& packageName) const {
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);
	auto it = m_appStats.find(packageName);
	AppLaunchStats stats = (it != m_appStats.end()) ? it->second : AppLaunchStats {};
	xSemaphoreGive((SemaphoreHandle_t)m_mutex);
	return stats;
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
		lockGui();
		activeApp->update();
		unlockGui();
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
	flx::core::Bundle data;
	data.putString("appId", appId);
	flx::core::EventBus::getInstance().publish(event, data);
}

void AppManager::recordLaunchMetrics(const std::string& packageName, int64_t startTimeUs, int32_t heapDeltaBytes) {
	auto& stats = m_appStats[packageName];
	stats.launchCount++;
	stats.lastStartTimeUs = startTimeUs;
	stats.heapDeltaBytes = heapDeltaBytes;
	stats.lastActiveTimeMs = 0;
}

void AppManager::recordActiveTime(AppStackEntry& entry, int64_t nowUs) {
	if (!entry.app || entry.activeSinceUs <= 0) {
		return;
	}

	uint32_t activeTimeMs = durationUsToMs(nowUs - entry.activeSinceUs);
	auto& stats = m_appStats[entry.app->getPackageName()];
	stats.lastActiveTimeMs = activeTimeMs;
	stats.totalActiveTimeMs = saturatingAddMs(stats.totalActiveTimeMs, activeTimeMs);
	entry.activeSinceUs = 0;
}

void AppManager::markTopAppActive(int64_t nowUs) {
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);
	if (!m_appStack.empty()) {
		m_appStack.back().activeSinceUs = nowUs;
	}
	xSemaphoreGive((SemaphoreHandle_t)m_mutex);
}

void AppManager::dumpAppStates() const {
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);

	Log::info("AppManager", "=== App States (%zu registered, %zu in stack) ===", m_apps.size(), m_appStack.size());
	for (const auto& app: m_apps) {
		if (!app) {
			continue;
		}

		const std::string packageName = app->getPackageName();
		const auto statsIt = m_appStats.find(packageName);
		const AppLaunchStats stats = (statsIt != m_appStats.end()) ? statsIt->second : AppLaunchStats {};

		const char* state = "registered";
		for (size_t i = 0; i < m_appStack.size(); ++i) {
			if (!m_appStack[i].app || m_appStack[i].app->getPackageName() != packageName) {
				continue;
			}
			state = (i == m_appStack.size() - 1) ? "active" : "paused";
			break;
		}

		Log::info("AppManager", "  [%s] %s - %s (launches: %lu, start: %lld ms, heap: %ld B, last active: %lu ms, total active: %lu ms)", state, app->getAppName().c_str(), packageName.c_str(), (unsigned long)stats.launchCount, (long long)(stats.lastStartTimeUs / 1000), (long)stats.heapDeltaBytes, (unsigned long)stats.lastActiveTimeMs, (unsigned long)stats.totalActiveTimeMs);
	}
	Log::info("AppManager", "==============================================");

	xSemaphoreGive((SemaphoreHandle_t)m_mutex);
}

void AppManager::performHealthCheck() {
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);

	size_t stackSize = m_appStack.size();
	std::string topApp = stackSize > 0 ? m_appStack.back().app->getPackageName() : "None";

	int appCount = (int)m_apps.size();
	Log::info("AppManager", "Health: %d apps registered, %zu in stack, Top: %s", appCount, stackSize, topApp.c_str());

	xSemaphoreGive((SemaphoreHandle_t)m_mutex);
}

// ============================================================
// Crash Recovery (2.5)
// ============================================================

void AppManager::reportAppCrash(const std::string& appId, const std::string& reason) {
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);

	auto& record = m_crashRecords[appId];
	record.appId = appId;
	record.crashCount++;
	record.lastError = reason;

	uint32_t now = static_cast<uint32_t>(esp_timer_get_time() / 1000000); // seconds
	record.timestamps[record.index % MAX_CRASHES_BEFORE_BLOCK] = now;
	record.index++;

	uint32_t currentCrashCount = record.crashCount;
	xSemaphoreGive((SemaphoreHandle_t)m_mutex);

	Log::warn("AppManager", "App '%s' crashed (#%lu): %s", appId.c_str(), (unsigned long)currentCrashCount, reason.c_str());

	publishAppEvent(flx::core::Events::APP_CRASHED, appId);

	if (isAppBlocked(appId)) {
		Log::error("AppManager", "App '%s' blocked after %d crashes in %d seconds", appId.c_str(), MAX_CRASHES_BEFORE_BLOCK, CRASH_WINDOW_SECONDS);
		publishAppEvent(flx::core::Events::APP_BLOCKED, appId);
	}
}

bool AppManager::isAppBlocked(const std::string& appId) const {
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);

	auto it = m_crashRecords.find(appId);
	if (it == m_crashRecords.end() || it->second.crashCount < MAX_CRASHES_BEFORE_BLOCK) {
		xSemaphoreGive((SemaphoreHandle_t)m_mutex);
		return false;
	}

	const auto& record = it->second;
	uint32_t now = static_cast<uint32_t>(esp_timer_get_time() / 1000000);

	// Check if all MAX_CRASHES_BEFORE_BLOCK crashes happened within the window
	uint32_t oldest_idx = (record.index + MAX_CRASHES_BEFORE_BLOCK - (MAX_CRASHES_BEFORE_BLOCK)) % MAX_CRASHES_BEFORE_BLOCK;
	uint32_t oldest = record.timestamps[oldest_idx];

	xSemaphoreGive((SemaphoreHandle_t)m_mutex);

	return (now - oldest) <= static_cast<uint32_t>(CRASH_WINDOW_SECONDS);
}

void AppManager::clearCrashHistory(const std::string& appId) {
	xSemaphoreTake((SemaphoreHandle_t)m_mutex, portMAX_DELAY);
	m_crashRecords.erase(appId);
	xSemaphoreGive((SemaphoreHandle_t)m_mutex);

	Log::info("AppManager", "Crash history cleared for '%s'", appId.c_str());
}

} // namespace flx::apps
