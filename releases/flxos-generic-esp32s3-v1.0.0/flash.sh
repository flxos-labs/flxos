#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PORT="${1:-}"

if [[ -z "$PORT" ]]; then
  echo "Usage: ./flash.sh /dev/ttyUSB0" >&2
  exit 1
fi

if command -v esptool >/dev/null 2>&1; then
  ESPTOOL=(esptool)
elif command -v esptool.py >/dev/null 2>&1; then
  ESPTOOL=(esptool.py)
else
  ESPTOOL=(python3 -m esptool)
fi

WRITE_ARGS="$(sed -n '1p' "$SCRIPT_DIR/flash_args")"
FLASH_FILES="$(sed -n '2p' "$SCRIPT_DIR/flash_args")"

"${ESPTOOL[@]}" --port "$PORT" erase_flash
# shellcheck disable=SC2086
"${ESPTOOL[@]}" --port "$PORT" write_flash $WRITE_ARGS $FLASH_FILES
