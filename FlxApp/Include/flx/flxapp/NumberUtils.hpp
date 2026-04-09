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

inline int64_t normalizeDecrementAmount(int64_t amount) {
    if (amount >= 0) {
        return amount;
    }
    return (amount == std::numeric_limits<int64_t>::min())
               ? std::numeric_limits<int64_t>::max()
               : -amount;
}

} // namespace flx::flxapp::detail
