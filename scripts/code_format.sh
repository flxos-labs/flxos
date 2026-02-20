#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

find \
    "$PROJECT_ROOT/System" \
    "$PROJECT_ROOT/UI" \
    "$PROJECT_ROOT/Connectivity" \
    "$PROJECT_ROOT/Kernel" \
    "$PROJECT_ROOT/Services" \
    "$PROJECT_ROOT/Core" \
    "$PROJECT_ROOT/Apps" \
    "$PROJECT_ROOT/Applications" \
    "$PROJECT_ROOT/Firmware" \
    "$PROJECT_ROOT/HAL" \
    "$PROJECT_ROOT/Profiles" \
    \( -name "*.cpp" -o -name "*.hpp" -o -name "*.c" -o -name "*.h" \) -print0 \
| xargs -0 clang-format -i

echo "FORMATTING COMPLETE"
