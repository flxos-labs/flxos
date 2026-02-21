#pragma once
#include <sdkconfig.h>

#ifndef CONFIG_FLXOS_HEADLESS_MODE
#error "The generic-esp32s3 profile only supports headless mode. Enable 'Headless Mode' in menuconfig."
#endif

// ============================================================================
// Device Profile Metadata
// ============================================================================
#define FLXOS_PROFILE_ID "generic-esp32s3"
#define FLXOS_PROFILE_VENDOR "Generic"
#define FLXOS_PROFILE_BOARD_NAME "ESP32-S3 (Headless)"

// Headless explicit overrides
#define FLXOS_ENABLE_DISPLAY false
#define FLXOS_TOUCH_ENABLED false
