#pragma once

#include <flx/core/GuiLock.hpp>

namespace flx::core {

/**
 * @brief RAII wrapper for GuiLock
 */
struct GuiLockGuard {
	GuiLockGuard() noexcept { flx::core::GuiLock::lock(); }
	~GuiLockGuard() noexcept { flx::core::GuiLock::unlock(); }

	GuiLockGuard(const GuiLockGuard&) = delete;
	GuiLockGuard& operator=(const GuiLockGuard&) = delete;
	GuiLockGuard(GuiLockGuard&&) = delete;
	GuiLockGuard& operator=(GuiLockGuard&&) = delete;
};

} // namespace flx::core
