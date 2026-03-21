#!/bin/bash
set -euo pipefail

if ! command -v idf.py >/dev/null 2>&1; then
    echo "Error: idf.py is not in PATH."
    echo "Please source your ESP-IDF export.sh environment first."
    exit 1
fi

# Save the current configuration to sdkconfig.defaults
idf.py save-defconfig
echo "Configuration saved to sdkconfig.defaults"
