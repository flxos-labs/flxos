#include <flx/flxapp/FlxAppLoader.hpp>

#include <dirent.h>
#include <flx/apps/AppManager.hpp>
#include <flx/apps/AppRegistry.hpp>
#include <flx/core/EventBus.hpp>
#include <flx/core/Logger.hpp>
#include <flx/flxapp/FlxApp.hpp>
#include <flx/flxapp/FlxAppManifest.hpp>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <sys/stat.h>

#include <vector>

namespace flx::flxapp {

namespace {

constexpr const char* TAG = "FlxAppLoader";
constexpr const char* DATA_APP_ROOT = "/data/apps";
constexpr const char* SD_APP_ROOT = "/sdcard/apps";
constexpr const char* SCAN_TASK_NAME = "flxapp_scan";
// Matches AppExecutor task stack size used for app/runtime call paths in this codebase.
constexpr uint32_t SCAN_TASK_STACK_WORDS = 8192;
// Keep this low so UI/system tasks stay responsive.
constexpr UBaseType_t SCAN_TASK_PRIORITY = tskIDLE_PRIORITY + 1;

bool hasSuffix(const std::string& value, const char* suffix) {
    const std::string ending = suffix;
    if (value.size() < ending.size()) {
        return false;
    }

    return value.compare(value.size() - ending.size(), ending.size(), ending) == 0;
}

bool isFlxAppFile(const std::string& path) {
    return hasSuffix(path, ".flxapp") ||
           hasSuffix(path, ".flxapp.json") ||
           hasSuffix(path, ".flxapp.yaml") ||
           hasSuffix(path, ".flxapp.yml");
}

bool isDirectory(const std::string& path) {
    struct stat st {};
    return ::stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

bool pathStartsWith(const std::string& path, const char* root) {
    const std::string prefix = root;
    return path.compare(0, prefix.size(), prefix) == 0;
}

} // namespace

void FlxAppLoader::init() {
    if (m_initialized) {
        return;
    }

    m_initialized = true;
    scheduleScan();

    m_sdMountedSubscription = flx::core::EventBus::getInstance().subscribe(
        flx::core::Events::SDCARD_MOUNTED,
        [this](const std::string& /*event*/, const flx::core::Bundle& /*data*/) {
            scheduleScan();
        });

    m_sdUnmountedSubscription = flx::core::EventBus::getInstance().subscribe(
        flx::core::Events::SDCARD_UNMOUNTED,
        [this](const std::string& /*event*/, const flx::core::Bundle& /*data*/) {
            unregisterAppsUnderRoot(SD_APP_ROOT);
        });

    flx::core::EventBus::getInstance().publish(flx::core::Events::FLXAPP_LOADER_READY, {});
}

void FlxAppLoader::scheduleScan() {
    m_scanPending.store(true);
    bool expected = false;
    if (!m_scanTaskRunning.compare_exchange_strong(expected, true)) {
        return;
    }

    auto* self = this;
    BaseType_t created = xTaskCreate(
        [](void* context) {
            auto* loader = static_cast<FlxAppLoader*>(context);
            for (;;) {
                while (loader->m_scanPending.exchange(false)) {
                    loader->scanAndRegister();
                }
                loader->m_scanTaskRunning.store(false);
                if (!loader->m_scanPending.load()) {
                    break;
                }
                bool expected = false;
                if (!loader->m_scanTaskRunning.compare_exchange_strong(expected, true)) {
                    break;
                }
            }
            vTaskDelete(nullptr);
        },
        SCAN_TASK_NAME,
        SCAN_TASK_STACK_WORDS,
        self,
        SCAN_TASK_PRIORITY,
        nullptr);

    if (created != pdPASS) {
        m_scanTaskRunning.store(false);
        Log::error(TAG, "Failed to start FlxApp scan task");
        scanAndRegister();
    }
}

void FlxAppLoader::scanAndRegister() {
    const char* roots[] = {DATA_APP_ROOT, SD_APP_ROOT};

    for (const char* root: roots) {
        DIR* dir = ::opendir(root);
        if (dir == nullptr) {
            continue;
        }

        while (dirent* entry = ::readdir(dir)) {
            const std::string name = entry->d_name;
            if (name.empty() || name == "." || name == "..") {
                continue;
            }

            const std::string fullPath = std::string(root) + "/" + name;
            if (isFlxAppFile(name)) {
                registerAppFile(fullPath.c_str());
                continue;
            }

            if (isDirectory(fullPath)) {
                registerAppFile((fullPath + "/app.flxapp").c_str());
                registerAppFile((fullPath + "/app.flxapp.json").c_str());
                registerAppFile((fullPath + "/app.flxapp.yaml").c_str());
                registerAppFile((fullPath + "/app.flxapp.yml").c_str());
            }
        }

        ::closedir(dir);
    }
}

void FlxAppLoader::registerAppFile(const char* path) {
    if (path == nullptr) {
        return;
    }

    struct stat st {};
    if (::stat(path, &st) != 0 || !S_ISREG(st.st_mode)) {
        return;
    }

    const std::string sourcePath = path;
    if (m_registeredPaths.find(sourcePath) != m_registeredPaths.end()) {
        return;
    }

    auto manifest = FlxAppManifest::loadFromFile(sourcePath);
    if (!manifest) {
        return;
    }

    if (flx::apps::AppRegistry::getInstance().hasApp(manifest->appId)) {
        Log::warn(TAG, "Skipping FlxApp with duplicate appId '%s' from %s",
                  manifest->appId.c_str(), sourcePath.c_str());
        return;
    }

    manifest->createApp = [sourcePath]() -> std::shared_ptr<flx::apps::App> {
        return std::make_shared<FlxApp>(sourcePath);
    };

    flx::apps::AppRegistry::getInstance().addApp(*manifest);
    m_registeredAppsByPath[sourcePath] = manifest->appId;
    m_registeredPaths.insert(sourcePath);

    flx::core::Bundle data;
    data.putString("appId", manifest->appId);
    data.putString("path", sourcePath);
    flx::core::EventBus::getInstance().publish(flx::core::Events::FLXAPP_LOADED, data);
    Log::info(TAG, "Loaded FlxApp: %s from %s", manifest->appId.c_str(), sourcePath.c_str());
}

void FlxAppLoader::unregisterAppsUnderRoot(const char* rootPath) {
    if (rootPath == nullptr) {
        return;
    }

    std::vector<std::string> pathsToRemove {};
    pathsToRemove.reserve(m_registeredAppsByPath.size());

    for (const auto& [path, appId]: m_registeredAppsByPath) {
        if (!pathStartsWith(path, rootPath)) {
            continue;
        }

        flx::apps::AppManager::getInstance().stopApp(appId, true);
        flx::apps::AppRegistry::getInstance().removeApp(appId);
        pathsToRemove.push_back(path);
        Log::info(TAG, "Unloaded FlxApp: %s from %s", appId.c_str(), path.c_str());
    }

    for (const auto& path: pathsToRemove) {
        m_registeredAppsByPath.erase(path);
        m_registeredPaths.erase(path);
    }
}

} // namespace flx::flxapp
