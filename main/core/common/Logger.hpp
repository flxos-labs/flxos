#pragma once
#include "esp_log.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>

class Log {
public:

	static void info(const char* tag, const char* fmt, ...) __attribute__((format(printf, 2, 3))) {
		va_list args;
		va_start(args, fmt);
		log_impl(ESP_LOG_INFO, "I", "\033[0;32m", tag, fmt, args);
		va_end(args);
	}

	static void error(const char* tag, const char* fmt, ...) __attribute__((format(printf, 2, 3))) {
		va_list args;
		va_start(args, fmt);
		log_impl(ESP_LOG_ERROR, "E", "\033[0;31m", tag, fmt, args);
		va_end(args);
	}

	static void warn(const char* tag, const char* fmt, ...) __attribute__((format(printf, 2, 3))) {
		va_list args;
		va_start(args, fmt);
		log_impl(ESP_LOG_WARN, "W", "\033[0;33m", tag, fmt, args);
		va_end(args);
	}

	static void debug(const char* tag, const char* fmt, ...) __attribute__((format(printf, 2, 3))) {
		va_list args;
		va_start(args, fmt);
		log_impl(ESP_LOG_DEBUG, "D", "\033[0;37m", tag, fmt, args);
		va_end(args);
	}

	static void verbose(const char* tag, const char* fmt, ...) __attribute__((format(printf, 2, 3))) {
		va_list args;
		va_start(args, fmt);
		log_impl(ESP_LOG_VERBOSE, "V", "\033[0;38;5;250m", tag, fmt, args);
		va_end(args);
	}

private:

	static void log_impl(esp_log_level_t level, const char* level_char, const char* color, const char* tag, const char* fmt, va_list args) {
		char buf[1024];
		int pos = std::snprintf(buf, sizeof(buf), "%s%s (%lu) %s: ", color, level_char, (unsigned long)esp_log_timestamp(), tag);
		if (pos >= 0 && pos < (int)sizeof(buf)) {
			int msg_len = std::vsnprintf(buf + pos, sizeof(buf) - pos - 8, fmt, args); // -8 for reset sequence and newline
			if (msg_len >= 0) {
				std::strcat(buf, "\033[0m\n");
				esp_log_write(level, tag, "%s", buf);
			}
		}
	}
};
