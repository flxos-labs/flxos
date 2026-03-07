#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
CLANG_FORMAT="${CLANG_FORMAT:-clang-format}"

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
    "$PROJECT_ROOT/HalModule" \
    "$PROJECT_ROOT/Profiles" \
    \( -name "*.cpp" -o -name "*.hpp" -o -name "*.c" -o -name "*.h" \) -print0 \
| xargs -0 $CLANG_FORMAT -i

echo "FORMATTING COMPLETE"
