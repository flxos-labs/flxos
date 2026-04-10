#pragma once

#include <flx/core/Value.hpp>

#include <functional>
#include <string>

namespace flx::flxapp {

class FlxApp;
class FlxAppState;

class FlxAppActionRunner {
public:

    FlxAppActionRunner(FlxApp& app, FlxAppState& state);

    void setRefreshCallback(std::function<void()> callback);
    void run(const flx::core::FlxValueView& actions);

private:

    FlxApp& m_app;
    FlxAppState& m_state;
    std::function<void()> m_refreshCallback {};

    void runSingle(const flx::core::FlxValueView& action);
    void notifyRefreshed();
    std::string resolveString(const flx::core::FlxValueView& value) const;
};

} // namespace flx::flxapp
