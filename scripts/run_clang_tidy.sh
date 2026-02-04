#!/bin/bash
# Run clang-tidy on FlxOS source files
# Requires compile_commands.json from build directory

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
REPORT_DIR="$PROJECT_ROOT/reports"

echo "=== FlxOS Clang-Tidy Analysis ==="

# Check for compile_commands.json
if [ ! -f "$BUILD_DIR/compile_commands.json" ]; then
    echo "⚠️  compile_commands.json not found!"
    echo "   Run 'idf.py build' first to generate it."
    exit 1
fi

# Create reports directory
mkdir -p "$REPORT_DIR"

REPORT_FILE="$REPORT_DIR/clang_tidy_report.txt"
SUMMARY_FILE="$REPORT_DIR/clang_tidy_summary.txt"

echo "Analyzing source files..."
echo "Report will be saved to: $REPORT_FILE"

# Counter for issues
TOTAL_WARNINGS=0
TOTAL_ERRORS=0

# Clear previous report
> "$REPORT_FILE"


# Run clang-tidy on each source file
while IFS= read -r -d '' file; do
    echo "Checking: ${file#$PROJECT_ROOT/}"

    # Run clang-tidy and capture output
    OUTPUT=$(clang-tidy -p "$BUILD_DIR" "$file" 2>&1 || true)

    if [ -n "$OUTPUT" ]; then
        echo "=== $file ===" >> "$REPORT_FILE"
        echo "$OUTPUT" >> "$REPORT_FILE"
        echo "" >> "$REPORT_FILE"

        # Count warnings and errors
        WARNINGS=$(echo "$OUTPUT" | grep -c "warning:" || true)
        ERRORS=$(echo "$OUTPUT" | grep -c "error:" || true)
        TOTAL_WARNINGS=$((TOTAL_WARNINGS + WARNINGS))
        TOTAL_ERRORS=$((TOTAL_ERRORS + ERRORS))

        if [ "$WARNINGS" -gt 0 ] || [ "$ERRORS" -gt 0 ]; then
            echo "  ⚠️  $WARNINGS warnings, $ERRORS errors"
        fi
    fi
done < <(find "$PROJECT_ROOT/main" -name "*.cpp" -print0)

# Generate summary
{
    echo "=== Clang-Tidy Summary ==="
    echo "Date: $(date)"
    echo ""
    echo "Total Warnings: $TOTAL_WARNINGS"
    echo "Total Errors: $TOTAL_ERRORS"
    echo ""
    echo "Details in: $REPORT_FILE"
} | tee "$SUMMARY_FILE"

echo ""
if [ $TOTAL_ERRORS -gt 0 ]; then
    echo "❌ Analysis complete with errors. Check $REPORT_FILE for details."
    exit 1
elif [ $TOTAL_WARNINGS -gt 0 ]; then
    echo "⚠️  Analysis complete with warnings. Check $REPORT_FILE for details."
    exit 0
else
    echo "✅ No issues found!"
    exit 0
fi
