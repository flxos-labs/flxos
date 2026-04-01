#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
REQUIRED_CLANG_FORMAT_MAJOR=18

print_install_hint() {
    if [ -r /etc/os-release ]; then
        # shellcheck disable=SC1091
        . /etc/os-release
        case "${ID:-}" in
            arch)
                echo "Install hint (Arch): sudo pacman -S clang18"
                ;;
            ubuntu|debian)
                echo "Install hint (Ubuntu/Debian): sudo apt-get install -y clang-format-18"
                ;;
        esac
    fi
    echo "Or set CLANG_FORMAT to a clang-format 18 executable path."
}

if [ -z "${CLANG_FORMAT:-}" ]; then
    if command -v clang-format-18 >/dev/null 2>&1; then
        CLANG_FORMAT="clang-format-18"
    elif [ -x "/usr/lib/llvm18/bin/clang-format" ]; then
        CLANG_FORMAT="/usr/lib/llvm18/bin/clang-format"
    else
        CLANG_FORMAT="clang-format"
    fi
fi

if ! command -v "$CLANG_FORMAT" >/dev/null 2>&1; then
    echo "❌ clang-format tool not found: $CLANG_FORMAT"
    print_install_hint
    exit 1
fi

CLANG_FORMAT_VERSION="$($CLANG_FORMAT --version 2>/dev/null || true)"
CLANG_FORMAT_MAJOR="$(echo "$CLANG_FORMAT_VERSION" | sed -nE 's/.*version[[:space:]]+([0-9]+).*/\1/p' | head -n1)"

if [ -z "$CLANG_FORMAT_MAJOR" ] || [ "$CLANG_FORMAT_MAJOR" != "$REQUIRED_CLANG_FORMAT_MAJOR" ]; then
    echo "❌ clang-format major version mismatch."
    echo "Found: ${CLANG_FORMAT_VERSION:-unknown}"
    echo "Required: ${REQUIRED_CLANG_FORMAT_MAJOR}.x (matches CI formatter)"
    print_install_hint
    exit 1
fi

echo "Using: $CLANG_FORMAT ($CLANG_FORMAT_VERSION)"

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
