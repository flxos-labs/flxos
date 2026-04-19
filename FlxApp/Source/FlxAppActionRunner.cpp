#include <flx/flxapp/FlxAppActionRunner.hpp>

#include <flx/apps/AppManager.hpp>
#include <flx/apps/Intent.hpp>
#include <flx/core/Bundle.hpp>
#include <flx/core/EventBus.hpp>
#include <flx/core/Logger.hpp>
#include <flx/flxapp/FlxApp.hpp>
#include <flx/flxapp/FlxAppState.hpp>
#include <flx/flxapp/NumberUtils.hpp>

#include <esp_err.h>
#include <esp_http_client.h>

#include <algorithm>
#include <utility>

namespace flx::flxapp {

namespace {

constexpr const char* TAG = "FlxAppActionRunner";

struct HttpBodyAccumulator {
	std::string body {};
	size_t limit = 2048;
};

esp_err_t onHttpEvent(esp_http_client_event_t* event) {
	if (event == nullptr || event->user_data == nullptr) {
		return ESP_OK;
	}

	auto* accumulator = static_cast<HttpBodyAccumulator*>(event->user_data);
	if (event->event_id != HTTP_EVENT_ON_DATA || event->data == nullptr || event->data_len <= 0) {
		return ESP_OK;
	}

	if (accumulator->body.size() >= accumulator->limit) {
		return ESP_OK;
	}

	const size_t remaining = accumulator->limit - accumulator->body.size();
	const size_t copyLen = std::min(remaining, static_cast<size_t>(event->data_len));
	accumulator->body.append(static_cast<const char*>(event->data), copyLen);
	return ESP_OK;
}

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
	bool refreshViaState = false;

	if (type == "increment") {
		const int64_t amount = action.child("amount").asInt64(1);
		m_state.increment(key, flx::flxapp::detail::clampInt64ToInt32(amount));
		refreshViaState = true;
	} else if (type == "decrement") {
		const int64_t rawAmount = action.child("amount").asInt64(1);
		if (rawAmount < 0) {
			Log::warn(TAG, "FlxApp decrement amount is negative; using absolute value");
		}
		const int64_t amount = flx::flxapp::detail::normalizeDecrementAmount(rawAmount);
		const int32_t decrementBy = flx::flxapp::detail::clampInt64ToInt32(amount);
		m_state.increment(key, -decrementBy);
		refreshViaState = true;
	} else if (type == "toggle") {
		m_state.toggle(key);
		refreshViaState = true;
	} else if (type == "set") {
		m_state.setFromValue(key, action.child("value"));
		refreshViaState = true;
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
		return;
	} else if (type == "notify") {
		flx::core::Bundle data;
		data.putString("title", resolveString(action.child("title")));
		data.putString("message", resolveString(action.child("message")));
		data.putString("appName", m_app.getAppName());
		data.putString("icon", resolveString(action.child("icon")));
		flx::core::EventBus::getInstance().publish("system.notify", data);
	} else if (type == "http_get") {
		refreshViaState = runHttpGet(action, key);
	}

	if (!refreshViaState) {
		notifyRefreshed();
	}
}

bool FlxAppActionRunner::runHttpGet(const flx::core::FlxValueView& action, const std::string& fallbackResponseKey) {
	const std::string url = resolveString(action.child("url"));
	if (url.empty()) {
		Log::warn(TAG, "FlxApp http_get skipped because url is empty");
		return false;
	}

	int32_t timeoutMs = flx::flxapp::detail::clampInt64ToInt32(action.child("timeout_ms").asInt64(5000));
	if (timeoutMs <= 0) {
		timeoutMs = 5000;
	}

	int32_t maxBodyBytes = flx::flxapp::detail::clampInt64ToInt32(action.child("max_body_bytes").asInt64(2048));
	if (maxBodyBytes < 0) {
		maxBodyBytes = 0;
	}

	HttpBodyAccumulator accumulator {};
	accumulator.limit = static_cast<size_t>(maxBodyBytes);

	esp_http_client_config_t config = {};
	config.url = url.c_str();
	config.method = HTTP_METHOD_GET;
	config.timeout_ms = timeoutMs;
	config.event_handler = onHttpEvent;
	config.user_data = &accumulator;

	esp_http_client_handle_t client = esp_http_client_init(&config);
	if (client == nullptr) {
		Log::error(TAG, "FlxApp http_get failed to initialize client");
		return false;
	}

	const esp_err_t result = esp_http_client_perform(client);
	const int statusCode = esp_http_client_get_status_code(client);
	esp_http_client_cleanup(client);

	const std::string responseKey = action.child("response_key").asString(fallbackResponseKey);
	const std::string statusKey = action.child("status_key").asString();
	const std::string errorKey = action.child("error_key").asString();
	bool stateUpdated = false;

	if (!statusKey.empty()) {
		m_state.setInt(statusKey, statusCode);
		stateUpdated = true;
	}

	if (result == ESP_OK) {
		if (!responseKey.empty()) {
			m_state.setString(responseKey, accumulator.body);
			stateUpdated = true;
		}
		return stateUpdated;
	}

	Log::warn(TAG,
		"FlxApp http_get request failed for %s: %s",
		url.c_str(),
		esp_err_to_name(result));

	if (!errorKey.empty()) {
		m_state.setString(errorKey, esp_err_to_name(result));
		stateUpdated = true;
	}

	return stateUpdated;
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
