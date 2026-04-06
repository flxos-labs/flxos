#pragma once

#include <flx/core/EventBus.hpp>
#include <flx/core/Singleton.hpp>
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

    void registerAppFile(const char* path);

    bool m_initialized = false;
    flx::core::EventBus::SubscriptionId m_sdMountedSubscription = 0;
    std::unordered_set<std::string> m_registeredPaths {};
};

} // namespace flx::flxapp
