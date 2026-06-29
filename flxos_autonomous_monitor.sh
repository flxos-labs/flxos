#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="${PROJECT_DIR:-/home/akash/flxos-labs/flxos}"
LOG_FILE="${LOG_FILE:-$PROJECT_DIR/flxos_monitor.log}"
SUMMARY_JSON="${SUMMARY_JSON:-$PROJECT_DIR/flxos_monitor_summary.json}"
HOURS_BACK="${HOURS_BACK:-24}"

# Validate that HOURS_BACK is numeric
if [[ ! "$HOURS_BACK" =~ ^[0-9]+$ ]]; then
	echo "Error: HOURS_BACK must be a numeric integer: '$HOURS_BACK'" >&2
	exit 1
fi

cd "$PROJECT_DIR"

mkdir -p "$(dirname "$LOG_FILE")"

exec >>"$LOG_FILE" 2>&1

# ---------- styling ----------
if [[ -t 1 ]]; then
 RED=$'\033[31m'
 GREEN=$'\033[32m'
 YELLOW=$'\033[33m'
 BLUE=$'\033[34m'
 DIM=$'\033[2m'
 RESET=$'\033[0m'
else
 RED=""
 GREEN=""
 YELLOW=""
 BLUE=""
 DIM=""
 RESET=""
 fi

ts() { date '+%Y-%m-%d %H:%M:%S'; }

log() {
 echo "[$(ts)] $*"
}

section() {
 echo
 log "==== $1 ===="
}

ok() {
 log "${GREEN}OK${RESET} $*"
}

warn() {
 log "${YELLOW}WARN${RESET} $*"
}

fail() {
 log "${RED}FAIL${RESET} $*"
 }

have_cmd() {
 command -v "$1" >/dev/null 2>&1
}

safe_git() {
 git "$@" 2>/dev/null || true
}

trim_newlines() {
 tr -d '\r\n'
}

count_lines() {
 local files=()
 while IFS= read -r -d '' f; do
 files+=("$f")
 done < <(
  find Applications Apps UI Services System Core HalModule Connectivity \
  -type f \( -name '*.c' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) \
 -print0 2>/dev/null || true
 )

 if ((${#files[@]} == 0)); then
 echo 0
 return 0
 fi

 wc -l "${files[@]}" 2>/dev/null | tail -n 1 | awk '{print $1+0}'
}

json_escape() {
 local s="${1:-}"
 s=${s//\\/\\\\}
 s=${s//\"/\\\"}
 s=${s//$'\n'/\\n}
 s=${s//$'\r'/}
 s=${s//$'\t'/\\t}
 printf '%s' "$s"
}

echo "[$(ts)] ${BLUE}=== FlxOS Autonomous Repo Check ===${RESET}"

section "Repository state"
BRANCH="$(safe_git rev-parse --abbrev-ref HEAD)"
LAST_COMMIT="$(safe_git log -1 --oneline)"
GIT_STATUS_RAW="$(git status --porcelain 2>/dev/null || true)"

echo "Project: $PROJECT_DIR"
echo "Branch: ${BRANCH:-unknown}"
echo "Last commit: ${LAST_COMMIT:-unknown}"

if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
 echo "Git status:"
 if [[ -n "$GIT_STATUS_RAW" ]]; then
 echo "$GIT_STATUS_RAW" | head -20
 else
 echo "clean"
 fi
else
 warn "Not a git repository."
fi

section "Version"
if [[ -f version.txt ]]; then
 VERSION="$(trim_newlines < version.txt)"
 echo "Version: ${VERSION:-unknown}"
else
 VERSION="unknown"
 warn "version.txt not found."
fi

section "Formatting"
FORMAT_STATUS=0
FORMAT_DETAIL="passed"

if have_cmd clang-format-18; then
 export CLANG_FORMAT="$(command -v clang-format-18)"
elif [[ -x /usr/lib/llvm18/bin/clang-format ]]; then
 export CLANG_FORMAT="/usr/lib/llvm18/bin/clang-format"
else
 CLANG_FORMAT=""
fi

if [[ -n "${CLANG_FORMAT:-}" ]]; then
 if [[ -x ./scripts/code_format.sh ]]; then
 if ./scripts/code_format.sh; then
 ok "Formatting check passed"
 else
 FORMAT_STATUS=$?
 FORMAT_DETAIL="failed with exit $FORMAT_STATUS"
 fail "Formatting check failed"
 fi
 else
 FORMAT_STATUS=2
 FORMAT_DETAIL="script missing"
 warn "Formatting script not found: ./scripts/code_format.sh"
 fi
else
 FORMAT_STATUS=3
 FORMAT_DETAIL="clang-format 18 unavailable"
 warn "clang-format 18 not found. Skipping formatting."
fi

section "Code quality"
QUALITY_STATUS=0
QUALITY_DETAIL="passed"

if [[ -x ./scripts/code_quality.sh ]]; then
 if ./scripts/code_quality.sh; then
 ok "Code quality check passed"
 else
 QUALITY_STATUS=$?
 QUALITY_DETAIL="failed with exit $QUALITY_STATUS"
 fail "Code quality check failed"
 fi
else
 QUALITY_STATUS=2
 QUALITY_DETAIL="script missing"
 warn "Quality script not found: ./scripts/code_quality.sh"
fi

section "Project size"
LOC_TOTAL="$(count_lines)"
echo "Total LOC in core directories: $LOC_TOTAL"

section "Recent changes"
RECENT_FILES=""
if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
 RECENT_FILES="$(
 git log --since="${HOURS_BACK} hours ago" --name-only --pretty=format: 2>/dev/null \
 | sed '/^$/d' | sort -u | head -20 || true
 )"
 if [[ -n "$RECENT_FILES" ]]; then
 echo "Files changed in the last ${HOURS_BACK} hours:"
 echo "$RECENT_FILES"
 else
 echo "No recent file changes found."
 fi
else
 warn "Skipping recent changes: not a git repo."
fi

section "Summary"
if [[ -z "$GIT_STATUS_RAW" ]]; then
 GIT_STATUS="clean"
else
 GIT_STATUS="has changes"
fi

if [[ "$FORMAT_STATUS" -eq 0 && "$QUALITY_STATUS" -eq 0 ]]; then
 RESULT="ALL FAST CHECKS PASSED"
 RESULT_COLOR="${GREEN}"
else
 RESULT="SOME CHECKS FAILED OR WERE SKIPPED"
 RESULT_COLOR="${YELLOW}"
fi

echo "Completed: $(ts)"
echo "Branch: ${BRANCH:-unknown}"
echo "Last commit: ${LAST_COMMIT:-unknown}"
echo "Git status: $GIT_STATUS"
echo "Version: $VERSION"
echo "Formatting status: $FORMAT_STATUS ($FORMAT_DETAIL)"
echo "Quality status: $QUALITY_STATUS ($QUALITY_DETAIL)"
echo "LOC total: $LOC_TOTAL"
echo "Result: $RESULT"

if [[ "$FORMAT_STATUS" -eq 0 && "$QUALITY_STATUS" -eq 0 ]]; then
 echo "[$(ts)] ${GREEN}=== Check Complete ===${RESET}"
else
 echo "[$(ts)] ${YELLOW}=== Check Complete ===${RESET}"
fi

cat > "$SUMMARY_JSON" <<EOF
{
 "timestamp": "$(json_escape "$(ts)")",
 "project_dir": "$(json_escape "$PROJECT_DIR")",
 "branch": "$(json_escape "${BRANCH:-unknown}")",
 "last_commit": "$(json_escape "${LAST_COMMIT:-unknown}")",
 "git_status": "$(json_escape "$GIT_STATUS")",
 "version": "$(json_escape "$VERSION")",
 "format_status": $FORMAT_STATUS,
 "format_detail": "$(json_escape "$FORMAT_DETAIL")",
 "quality_status": $QUALITY_STATUS,
 "quality_detail": "$(json_escape "$QUALITY_DETAIL")",
 "loc_total": $LOC_TOTAL,
 "result": "$(json_escape "$RESULT")",
 "hours_back": $HOURS_BACK
}
EOF