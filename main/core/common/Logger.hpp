#pragma once
#include "esp_log.h"

class Log {
public:

	static Log& instance() {
		static Log instance;
		return instance;
	}

	template<typename... Args>
	void info(const char* tag, const char* fmt, Args... args) {
		ESP_LOGI(tag, fmt, args...);
	}

	template<typename... Args>
	void error(const char* tag, const char* fmt, Args... args) {
		ESP_LOGE(tag, fmt, args...);
	}

	template<typename... Args>
	void warn(const char* tag, const char* fmt, Args... args) {
		ESP_LOGW(tag, fmt, args...);
	}

	template<typename... Args>
	void debug(const char* tag, const char* fmt, Args... args) {
		ESP_LOGD(tag, fmt, args...);
	}

	template<typename... Args>
	void verbose(const char* tag, const char* fmt, Args... args) {
		ESP_LOGV(tag, fmt, args...);
	}

private:

	Log() = default;
	Log(const Log&) = delete;
	Log& operator=(const Log&) = delete;
};
