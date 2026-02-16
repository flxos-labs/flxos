#pragma once
#include "esp_log.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string_view>

namespace flx {

class Log {
public:

	static void info(std::string_view tag, const char* fmt, ...) __attribute__((format(printf, 2, 3))) {
		va_list args;
		va_start(args, fmt);
		log_impl(ESP_LOG_INFO, "I", "\033[0;32m", tag, fmt, args);
		va_end(args);
	}

	static void error(std::string_view tag, const char* fmt, ...) __attribute__((format(printf, 2, 3))) {
		va_list args;
		va_start(args, fmt);
		log_impl(ESP_LOG_ERROR, "E", "\033[0;31m", tag, fmt, args);
		va_end(args);
	}

	static void warn(std::string_view tag, const char* fmt, ...) __attribute__((format(printf, 2, 3))) {
		va_list args;
		va_start(args, fmt);
		log_impl(ESP_LOG_WARN, "W", "\033[0;33m", tag, fmt, args);
		va_end(args);
	}

	static void debug(std::string_view tag, const char* fmt, ...) __attribute__((format(printf, 2, 3))) {
		va_list args;
		va_start(args, fmt);
		log_impl(ESP_LOG_DEBUG, "D", "\033[0;37m", tag, fmt, args);
		va_end(args);
	}

	static void verbose(std::string_view tag, const char* fmt, ...) __attribute__((format(printf, 2, 3))) {
		va_list args;
		va_start(args, fmt);
		log_impl(ESP_LOG_VERBOSE, "V", "\033[0;38;5;250m", tag, fmt, args);
		va_end(args);
	}

private:

	static void log_impl(esp_log_level_t level, const char* level_char, const char* color, std::string_view tag, const char* fmt, va_list args) {
		char buf[1024];
		int pos = std::snprintf(buf, sizeof(buf), "%s%s (%lu) %.*s: ", color, level_char, (unsigned long)esp_log_timestamp(), (int)tag.size(), tag.data());
		if (pos >= 0 && pos < (int)sizeof(buf) - 8) {
			int msg_len = std::vsnprintf(buf + pos, sizeof(buf) - (size_t)pos - 8, fmt, args); // -8 for reset sequence and newline
			if (msg_len >= 0) {
				std::strcat(buf, "\033[0m\n");
				// Provide a null-terminated tag for ESP-IDF's filtering system
				char tag_buf[32];
				std::size_t tag_len = std::min(tag.size(), sizeof(tag_buf) - 1);
				std::memcpy(tag_buf, tag.data(), tag_len);
				tag_buf[tag_len] = '\0';
				esp_log_write(level, tag_buf, "%s", buf);
			}
		}
	}
};

} // namespace flx

// Backward compatibility alias
using Log = flx::Log;
