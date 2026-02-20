#ifndef ESP_LOG_H
#define ESP_LOG_H

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

typedef enum {
	ESP_LOG_NONE,
	ESP_LOG_ERROR,
	ESP_LOG_WARN,
	ESP_LOG_INFO,
	ESP_LOG_DEBUG,
	ESP_LOG_VERBOSE
} esp_log_level_t;

inline unsigned long esp_log_timestamp() {
	return (unsigned long)time(NULL);
}

inline void esp_log_write(esp_log_level_t level, const char* tag, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

#endif
