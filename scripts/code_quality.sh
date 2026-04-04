#!/bin/bash
# Master script for FlxOS code quality checks
# Runs all quality analysis tools and generates a unified report

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
REPORT_DIR="$PROJECT_ROOT/reports"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Create reports directory
mkdir -p "$REPORT_DIR"

REPORT_FILE="$REPORT_DIR/code_quality_report.md"
TOTAL_ISSUES=0

echo -e "${BLUE}╔════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║       FlxOS Code Quality Analysis          ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════╝${NC}"
echo ""
echo "Report will be saved to: $REPORT_FILE"
echo ""

# Start report
{
    echo "# FlxOS Code Quality Report"
    echo ""
    echo "Generated: $(date)"
    echo ""
    echo "---"
    echo ""
} > "$REPORT_FILE"

# Helper function to run a check
run_check() {
    local name="$1"
    local cmd="$2"
    local output_file="$REPORT_DIR/${3}.txt"
    
    echo -e "${YELLOW}▶ Running: ${name}...${NC}"
    
    if eval "$cmd" > "$output_file" 2>&1; then
        echo -e "${GREEN}  ✓ Passed${NC}"
        echo "## $name" >> "$REPORT_FILE"
        echo "" >> "$REPORT_FILE"
        echo "✅ **Passed** - No issues found" >> "$REPORT_FILE"
        echo "" >> "$REPORT_FILE"
    else
        local issue_count=$(grep -c "⚠️\|❌\|warning\|error" "$output_file" 2>/dev/null || echo "0")
        echo -e "${RED}  ✗ Issues found${NC}"
        TOTAL_ISSUES=$((TOTAL_ISSUES + 1))
        
        echo "## $name" >> "$REPORT_FILE"
        echo "" >> "$REPORT_FILE"
        echo "⚠️ **Issues Found**" >> "$REPORT_FILE"
        echo "" >> "$REPORT_FILE"
        echo '```' >> "$REPORT_FILE"
        cat "$output_file" >> "$REPORT_FILE"
        echo '```' >> "$REPORT_FILE"
        echo "" >> "$REPORT_FILE"
        echo "[Full report]($output_file)" >> "$REPORT_FILE"
        echo "" >> "$REPORT_FILE"
    fi
}

# 1. Format Check
echo -e "\n${BLUE}═══ 1. Code Formatting ═══${NC}"
if [ -f "$SCRIPT_DIR/check_format.sh" ]; then
    run_check "Format Check" "$SCRIPT_DIR/check_format.sh" "format"
else
    echo -e "${YELLOW}  ⊘ Skipped (script not found)${NC}"
fi

# 2. Complexity Analysis
echo -e "\n${BLUE}═══ 2. Code Complexity ═══${NC}"
if [ -f "$SCRIPT_DIR/analyze_complexity.py" ]; then
    run_check "Complexity Analysis" "python3 $SCRIPT_DIR/analyze_complexity.py" "complexity"
else
    echo -e "${YELLOW}  ⊘ Skipped (script not found)${NC}"
fi

# 3. Include Dependencies
echo -e "\n${BLUE}═══ 3. Include Dependencies ═══${NC}"
if [ -f "$SCRIPT_DIR/analyze_includes.py" ]; then
    run_check "Include Analysis" "python3 $SCRIPT_DIR/analyze_includes.py" "includes"
else
    echo -e "${YELLOW}  ⊘ Skipped (script not found)${NC}"
fi

# 4. Naming Conventions
echo -e "\n${BLUE}═══ 4. Naming Conventions ═══${NC}"
if [ -f "$SCRIPT_DIR/check_naming.py" ]; then
    run_check "Naming Check" "python3 $SCRIPT_DIR/check_naming.py" "naming"
else
    echo -e "${YELLOW}  ⊘ Skipped (script not found)${NC}"
fi

# 5. Documentation
echo -e "\n${BLUE}═══ 5. Documentation ═══${NC}"
if [ -f "$SCRIPT_DIR/check_docs.py" ]; then
    run_check "Documentation Check" "python3 $SCRIPT_DIR/check_docs.py" "docs"
else
    echo -e "${YELLOW}  ⊘ Skipped (script not found)${NC}"
fi

# 6. Hardcoded Values (existing script)
echo -e "\n${BLUE}═══ 6. Hardcoded Values ═══${NC}"
if [ -f "$SCRIPT_DIR/find_hardcoded.py" ]; then
    run_check "Hardcoded Values" "python3 $SCRIPT_DIR/find_hardcoded.py" "hardcoded"
else
    echo -e "${YELLOW}  ⊘ Skipped (script not found)${NC}"
fi

# 7. Wallpaper Preset Gates
echo -e "\n${BLUE}═══ 7. Wallpaper Preset Gates ═══${NC}"
if [ -f "$SCRIPT_DIR/run_wallpaper_assets.sh" ]; then
    run_check "Wallpaper Preset Assets" "$SCRIPT_DIR/run_wallpaper_assets.sh" "wallpaper_presets"
else
    echo -e "${YELLOW}  ⊘ Skipped (script not found)${NC}"
fi

# 8. Literal Color Guardrail
echo -e "\n${BLUE}═══ 8. Literal Color Guardrail ═══${NC}"
if [ -f "$SCRIPT_DIR/check_literal_colors.py" ]; then
    run_check "Literal Color Guardrail" "python3 $SCRIPT_DIR/check_literal_colors.py" "literal_colors"
else
    echo -e "${YELLOW}  ⊘ Skipped (script not found)${NC}"
fi

# Summary
echo ""
echo -e "${BLUE}═══════════════════════════════════════════════${NC}"
echo -e "${BLUE}                  SUMMARY                       ${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════${NC}"

{
    echo "---"
    echo ""
    echo "## Summary"
    echo ""
    echo "| Check | Status |"
    echo "|-------|--------|"
} >> "$REPORT_FILE"

if [ $TOTAL_ISSUES -eq 0 ]; then
    echo -e "${GREEN}All checks passed! 🎉${NC}"
    echo "| All Checks | ✅ Passed |" >> "$REPORT_FILE"
else
    echo -e "${YELLOW}${TOTAL_ISSUES} check(s) have issues. See report for details.${NC}"
    echo "| Checks with Issues | ⚠️ $TOTAL_ISSUES |" >> "$REPORT_FILE"
fi

echo ""
echo -e "Full report: ${BLUE}${REPORT_FILE}${NC}"
echo ""

exit $TOTAL_ISSUES
