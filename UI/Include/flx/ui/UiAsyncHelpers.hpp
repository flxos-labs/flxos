#pragma once

#include "misc/lv_async.h"
#include <flx/core/Logger.hpp>
#include <functional>
#include <string_view>
#include <utility>

namespace flx::ui {

/**
 * @brief Post a function to be executed on the LVGL UI thread.
 *
 * Heap-allocates the std::function and schedules it via lv_async_call().
 * If lv_async_call() fails (returns non-LV_RESULT_OK), the allocation is
 * freed immediately to prevent a memory leak.
 *
 * @param fn Callable to execute on the UI thread.
 * @return true if the call was successfully enqueued, false otherwise.
 */
inline bool postToUi(std::function<void()> fn) {
	auto* cb = new std::function<void()>(std::move(fn));
	lv_result_t const res = lv_async_call([](void* user_data) {
		auto* callback = static_cast<std::function<void()>*>(user_data);
		(*callback)();
		delete callback;
	}, cb);
	if (res != LV_RESULT_OK) {
		delete cb;
		return false;
	}
	return true;
}

} // namespace flx::ui
