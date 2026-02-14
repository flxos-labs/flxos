#!/usr/bin/env bash
# Script to update include paths from old relative paths to new <flx/...> paths

set -e

echo "Updating include paths in main/core/..."

# Update all .hpp and .cpp files
find main/core -type f \( -name "*.hpp" -o -name "*.cpp" \) -exec sed -i \
    -e 's|#include "core/common/Logger\.hpp"|#include <flx/core/Logger.hpp>|g' \
    -e 's|#include "core/common/Singleton\.hpp"|#include <flx/core/Singleton.hpp>|g' \
    -e 's|#include "core/common/Observable\.hpp"|#include <flx/core/Observable.hpp>|g' \
    -e 's|#include "core/common/Types\.hpp"|#include <flx/core/Types.hpp>|g' \
    -e 's|#include "core/common/ClipboardManager\.hpp"|#include <flx/core/ClipboardManager.hpp>|g' \
    {} +

echo "Include paths updated successfully!"
echo "Next: Update namespace references from System:: to flx::"
