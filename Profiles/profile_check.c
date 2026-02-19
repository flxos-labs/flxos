/**
 * @file profile_check.c
 * @brief Compile-time guard to ensure a valid device profile is selected.
 *
 * This file is only compiled when no profile is selected (FLXOS_PROFILE_NONE).
 * It triggers a build error with a helpful message directing the user to menuconfig.
 */
#include "sdkconfig.h"

#if CONFIG_FLXOS_PROFILE_NONE
#error "No device profile selected! Run 'idf.py menuconfig' and select a profile under: Hardware Configuration -> Device Profile"
#endif
