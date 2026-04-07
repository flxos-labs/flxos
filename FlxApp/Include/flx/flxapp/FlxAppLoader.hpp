#pragma once

#include <flx/core/EventBus.hpp>
#include <flx/core/Singleton.hpp>
#include <atomic>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace flx::flxapp {

class FlxAppLoader : public flx::Singleton<FlxAppLoader> {
    friend class flx::Singleton<FlxAppLoader>;

public:

    void init();
    void scanAndRegister();

private:

    FlxAppLoader() = default;
    ~FlxAppLoader() = default;

    void scheduleScan();
    void registerAppFile(const char* path);
    void unregisterAppsUnderRoot(const char* rootPath);

    bool m_initialized = false;
    flx::core::EventBus::SubscriptionId m_sdMountedSubscription = 0;
    flx::core::EventBus::SubscriptionId m_sdUnmountedSubscription = 0;
    std::unordered_map<std::string, std::string> m_registeredAppsByPath {};
    std::unordered_set<std::string> m_registeredPaths {};
    std::atomic<bool> m_scanTaskRunning {false};
    std::atomic<bool> m_scanPending {false};
};

} // namespace flx::flxapp
