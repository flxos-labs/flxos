#include <flx/flxapp/FlxAppActionRunner.hpp>

#include <flx/apps/AppManager.hpp>
#include <flx/apps/Intent.hpp>
#include <flx/core/Bundle.hpp>
#include <flx/core/EventBus.hpp>
#include <flx/core/Logger.hpp>
#include <flx/flxapp/FlxApp.hpp>
#include <flx/flxapp/FlxAppState.hpp>

#include <utility>

namespace flx::flxapp {

namespace {

constexpr const char* TAG = "FlxAppActionRunner";

} // namespace

FlxAppActionRunner::FlxAppActionRunner(FlxApp& app, FlxAppState& state)
    : m_app(app), m_state(state) {}

void FlxAppActionRunner::setRefreshCallback(std::function<void()> callback) {
    m_refreshCallback = std::move(callback);
}

void FlxAppActionRunner::run(const flx::core::FlxValueView& actions) {
    if (!actions.valid()) {
        return;
    }

    if (actions.isSeq()) {
        actions.forEachChild([this](const flx::core::FlxValueView& action) {
            runSingle(action);
        });
        return;
    }

    runSingle(actions);
}

void FlxAppActionRunner::runSingle(const flx::core::FlxValueView& action) {
    if (!action.valid() || !action.isMap()) {
        return;
    }

    const std::string type = action.child("type").asString();
    const std::string key = action.child("key").asString();

    if (type == "increment") {
        m_state.increment(key, static_cast<int32_t>(action.child("amount").asInt64(1)));
    } else if (type == "decrement") {
        m_state.increment(key, -static_cast<int32_t>(action.child("amount").asInt64(1)));
    } else if (type == "toggle") {
        m_state.toggle(key);
    } else if (type == "set") {
        m_state.setFromValue(key, action.child("value"));
    } else if (type == "log") {
        Log::info(TAG, "%s", resolveString(action.child("message")).c_str());
    } else if (type == "event_publish") {
        const std::string eventName = resolveString(action.child("event"));
        if (!eventName.empty()) {
            flx::core::Bundle data;
            data.putString("appId", m_app.getPackageName());
            flx::core::EventBus::getInstance().publish(eventName, data);
        }
    } else if (type == "navigate") {
        const std::string target = resolveString(action.child("target"));
        if (!target.empty()) {
            flx::apps::AppManager::getInstance().startApp(flx::apps::Intent::forApp(target));
        }
    } else if (type == "close") {
        flx::apps::AppManager::getInstance().stopApp(m_app.getPackageName());
    } else if (type == "notify") {
        flx::core::Bundle data;
        data.putString("title", resolveString(action.child("title")));
        data.putString("message", resolveString(action.child("message")));
        data.putString("appName", m_app.getAppName());
        data.putString("icon", resolveString(action.child("icon")));
        flx::core::EventBus::getInstance().publish("system.notify", data);
    }

    notifyRefreshed();
}

void FlxAppActionRunner::notifyRefreshed() {
    if (m_refreshCallback) {
        m_refreshCallback();
    }
}

std::string FlxAppActionRunner::resolveString(const flx::core::FlxValueView& value) const {
    if (!value.valid() || !value.hasValue() || value.isNull()) {
        return {};
    }

    if (value.isBoolScalar()) {
        return value.asBool() ? "true" : "false";
    }

    if (value.isIntScalar()) {
        return std::to_string(value.asInt64());
    }

    return m_state.resolve(value.asString());
}

} // namespace flx::flxapp
