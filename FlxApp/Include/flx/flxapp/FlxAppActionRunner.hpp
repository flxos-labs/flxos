#pragma once

#include <functional>
#include <string>

struct cJSON;

namespace flx::flxapp {

class FlxApp;
class FlxAppState;

class FlxAppActionRunner {
public:

    FlxAppActionRunner(FlxApp& app, FlxAppState& state);

    void setRefreshCallback(std::function<void()> callback);
    void run(const cJSON* actions);

private:

    FlxApp& m_app;
    FlxAppState& m_state;
    std::function<void()> m_refreshCallback {};

    void runSingle(const cJSON* action);
    void notifyRefreshed();
    std::string resolveString(const cJSON* value) const;
};

} // namespace flx::flxapp
