#include "Logger.hpp"
#include <cstdarg>
#include <cstdio>

void Logger_mock_log(const char* tag, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	printf("[%s] ", tag);
	vprintf(fmt, args);
	printf("\n");
	va_end(args);
}
// This is a simplified approach, in reality I might need to modify Logger.cpp
// to be more platform-agnostic or provide a full mock implementation here.
