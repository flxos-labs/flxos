#!/bin/bash
set -euo pipefail
# Save the current configuration to sdkconfig.defaults
idf.py save-defconfig
echo "Configuration saved to sdkconfig.defaults"
