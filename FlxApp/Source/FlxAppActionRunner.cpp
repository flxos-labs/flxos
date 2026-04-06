#include <flx/flxapp/FlxAppActionRunner.hpp>

#include <cJSON.h>
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

std::string jsonString(const cJSON* object, const char* key) {
    if (!cJSON_IsObject(object)) {
        return {};
    }

    const cJSON* item = cJSON_GetObjectItemCaseSensitive(object, key);
    if (cJSON_IsString(item) && item->valuestring != nullptr) {
        return item->valuestring;
    }

    return {};
}

int jsonInt(const cJSON* object, const char* key, int fallback = 0) {
    if (!cJSON_IsObject(object)) {
        return fallback;
    }

    const cJSON* item = cJSON_GetObjectItemCaseSensitive(object, key);
    if (cJSON_IsNumber(item)) {
        return item->valueint;
    }

    return fallback;
}

} // namespace

FlxAppActionRunner::FlxAppActionRunner(FlxApp& app, FlxAppState& state)
    : m_app(app), m_state(state) {}

void FlxAppActionRunner::setRefreshCallback(std::function<void()> callback) {
    m_refreshCallback = std::move(callback);
}

void FlxAppActionRunner::run(const cJSON* actions) {
    if (actions == nullptr) {
        return;
    }

    if (cJSON_IsArray(actions)) {
        const cJSON* action = nullptr;
        cJSON_ArrayForEach(action, actions) {
            runSingle(action);
        }
        return;
    }

    runSingle(actions);
}

void FlxAppActionRunner::runSingle(const cJSON* action) {
    if (!cJSON_IsObject(action)) {
        return;
    }

    const std::string type = jsonString(action, "type");
    const std::string key = jsonString(action, "key");

    if (type == "increment") {
        m_state.increment(key, jsonInt(action, "amount", 1));
    } else if (type == "decrement") {
        m_state.increment(key, -jsonInt(action, "amount", 1));
    } else if (type == "toggle") {
        m_state.toggle(key);
    } else if (type == "set") {
        const cJSON* value = cJSON_GetObjectItemCaseSensitive(action, "value");
        m_state.setFromJson(key, value);
    } else if (type == "log") {
        Log::info(TAG, "%s", resolveString(cJSON_GetObjectItemCaseSensitive(action, "message")).c_str());
    } else if (type == "event_publish") {
        const std::string eventName = resolveString(cJSON_GetObjectItemCaseSensitive(action, "event"));
        if (!eventName.empty()) {
            flx::core::Bundle data;
            data.putString("appId", m_app.getPackageName());
            flx::core::EventBus::getInstance().publish(eventName, data);
        }
    } else if (type == "navigate") {
        const std::string target = resolveString(cJSON_GetObjectItemCaseSensitive(action, "target"));
        if (!target.empty()) {
            flx::apps::AppManager::getInstance().startApp(flx::apps::Intent::forApp(target));
        }
    } else if (type == "close") {
        flx::apps::AppManager::getInstance().stopApp(m_app.getPackageName());
    } else if (type == "notify") {
        flx::core::Bundle data;
        data.putString("title", resolveString(cJSON_GetObjectItemCaseSensitive(action, "title")));
        data.putString("message", resolveString(cJSON_GetObjectItemCaseSensitive(action, "message")));
        data.putString("appName", m_app.getAppName());
        data.putString("icon", resolveString(cJSON_GetObjectItemCaseSensitive(action, "icon")));
        flx::core::EventBus::getInstance().publish("system.notify", data);
    }

    notifyRefreshed();
}

void FlxAppActionRunner::notifyRefreshed() {
    if (m_refreshCallback) {
        m_refreshCallback();
    }
}

std::string FlxAppActionRunner::resolveString(const cJSON* value) const {
    if (cJSON_IsString(value) && value->valuestring != nullptr) {
        return m_state.resolve(value->valuestring);
    }
    if (cJSON_IsNumber(value)) {
        return std::to_string(value->valueint);
    }
    if (cJSON_IsBool(value)) {
        return cJSON_IsTrue(value) ? "true" : "false";
    }
    return {};
}

} // namespace flx::flxapp
