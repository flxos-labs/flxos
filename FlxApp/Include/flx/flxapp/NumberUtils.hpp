#pragma once

#include <cstdint>
#include <limits>

namespace flx::flxapp::detail {

inline int32_t clampInt64ToInt32(int64_t value) {
    if (value > std::numeric_limits<int32_t>::max()) {
        return std::numeric_limits<int32_t>::max();
    }
    if (value < std::numeric_limits<int32_t>::min()) {
        return std::numeric_limits<int32_t>::min();
    }
    return static_cast<int32_t>(value);
}

} // namespace flx::flxapp::detail
