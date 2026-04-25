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
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lvgl.h>

#include <algorithm>
#include <memory>
#include <utility>

namespace flx::flxapp {

namespace {

constexpr const char* TAG = "FlxAppActionRunner";

struct HttpBodyAccumulator {
	std::string body {};
	size_t limit = 2048;
};

struct HttpGetAsyncContext {
	std::string appId {};
	std::string url {};
	std::string responseKey {};
	std::string statusKey {};
	std::string errorKey {};
	int32_t timeoutMs = 5000;
	int32_t maxBodyBytes = 2048;
	int32_t statusCode = -1;
	bool success = false;
	std::string responseBody {};
	std::string errorMessage {};
};

constexpr const char* HTTP_GET_TASK_NAME = "flxapp_http_get";
constexpr uint32_t HTTP_GET_TASK_STACK_WORDS = 6 * 1024;
constexpr UBaseType_t HTTP_GET_TASK_PRIORITY = 4;

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

void runHttpGetRequest(HttpGetAsyncContext& context) {
	HttpBodyAccumulator accumulator {};
	accumulator.limit = static_cast<size_t>(context.maxBodyBytes);

	esp_http_client_config_t config = {};
	config.url = context.url.c_str();
	config.method = HTTP_METHOD_GET;
	config.timeout_ms = context.timeoutMs;
	config.event_handler = onHttpEvent;
	config.user_data = &accumulator;

	esp_http_client_handle_t client = esp_http_client_init(&config);
	if (client == nullptr) {
		context.success = false;
		context.errorMessage = "ESP_ERR_NO_MEM";
		return;
	}

	const esp_err_t result = esp_http_client_perform(client);
	context.statusCode = esp_http_client_get_status_code(client);
	esp_http_client_cleanup(client);

	context.success = (result == ESP_OK);
	if (context.success) {
		context.responseBody = std::move(accumulator.body);
		return;
	}

	context.errorMessage = esp_err_to_name(result);
	Log::warn(
		TAG,
		"FlxApp http_get request failed for %s: %s",
		context.url.c_str(),
		context.errorMessage.c_str());
}

void applyHttpGetResultAsync(void* userData) {
	std::unique_ptr<HttpGetAsyncContext> context(static_cast<HttpGetAsyncContext*>(userData));
	if (!context) {
		return;
	}

	std::shared_ptr<flx::apps::App> app = flx::apps::AppManager::getInstance().getAppByPackageName(context->appId);
	if (!app) {
		return;
	}

	std::shared_ptr<flx::flxapp::FlxApp> flxApp = std::dynamic_pointer_cast<flx::flxapp::FlxApp>(app);
	if (!flxApp) {
		return;
	}

	flxApp->applyHttpGetResult(
		context->responseKey,
		context->statusKey,
		context->errorKey,
		context->statusCode,
		context->success,
		context->responseBody,
		context->errorMessage);
}

void runHttpGetTask(void* taskArg) {
	std::unique_ptr<HttpGetAsyncContext> context(static_cast<HttpGetAsyncContext*>(taskArg));
	if (!context) {
		vTaskDelete(nullptr);
		return;
	}

	runHttpGetRequest(*context);

	HttpGetAsyncContext* rawContext = context.release();
	if (lv_async_call(applyHttpGetResultAsync, rawContext) != LV_RESULT_OK) {
		delete rawContext;
		Log::warn(TAG, "Failed to schedule FlxApp http_get completion callback");
	}

	vTaskDelete(nullptr);
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

	const std::string responseKey = action.child("response_key").asString(fallbackResponseKey);
	const std::string statusKey = action.child("status_key").asString();
	const std::string errorKey = action.child("error_key").asString();

	auto context = std::make_unique<HttpGetAsyncContext>();
	context->appId = m_app.getPackageName();
	context->url = url;
	context->responseKey = responseKey;
	context->statusKey = statusKey;
	context->errorKey = errorKey;
	context->timeoutMs = timeoutMs;
	context->maxBodyBytes = maxBodyBytes;

	if (xTaskCreate(
			runHttpGetTask,
			HTTP_GET_TASK_NAME,
			HTTP_GET_TASK_STACK_WORDS,
			context.get(),
			HTTP_GET_TASK_PRIORITY,
			nullptr) != pdPASS) {
		Log::warn(TAG, "FlxApp http_get failed to start async task; running synchronously");
		runHttpGetRequest(*context);
		m_app.applyHttpGetResult(
			context->responseKey,
			context->statusKey,
			context->errorKey,
			context->statusCode,
			context->success,
			context->responseBody,
			context->errorMessage);
		return !context->statusKey.empty() ||
			(context->success && !context->responseKey.empty()) ||
			(!context->success && !context->errorKey.empty());
	}

	context.release();
	return false;
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
