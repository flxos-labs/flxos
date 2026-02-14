#!/bin/bash
# Check code formatting without modifying files
# Exit 1 if any file needs formatting

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "=== FlxOS Format Check ==="
echo "Checking formatting in main/ directory..."

# Find all source files and check formatting
FAILED=0
while IFS= read -r -d '' file; do
    if ! clang-format --dry-run --Werror "$file" 2>/dev/null; then
        echo "❌ Needs formatting: $file"
        FAILED=1
    fi
done < <(find "$PROJECT_ROOT/main" "$PROJECT_ROOT/Apps" "$PROJECT_ROOT/Kernel" "$PROJECT_ROOT/Services" "$PROJECT_ROOT/Core" \( -name "*.cpp" -o -name "*.hpp" -o -name "*.c" -o -name "*.h" \) -print0)

if [ $FAILED -eq 0 ]; then
    echo "✅ All files are properly formatted!"
    exit 0
else
    echo ""
    echo "Run './scripts/code_format.sh' to fix formatting issues."
    exit 1
fi
