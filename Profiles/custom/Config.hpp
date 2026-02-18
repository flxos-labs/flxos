#pragma once

#include "sdkconfig.h"

// Map Kconfig values to standard Profile constants

#if CONFIG_FLXOS_ENABLE_DISPLAY
    #define FLXOS_DISPLAY_WIDTH         CONFIG_FLXOS_DISPLAY_WIDTH
    #define FLXOS_DISPLAY_HEIGHT        CONFIG_FLXOS_DISPLAY_HEIGHT
    
    // Config provides 0, 90, 180, 270.
    // LGFX and LVGL expect 0-3 for rotation? Or degrees?
    // GuiTask.cpp does: (lv_display_rotation_t)(CONFIG_FLXOS_DISPLAY_ROTATION / 90)
    // So preserving degrees is correct.
    #define FLXOS_DISPLAY_ROTATION      CONFIG_FLXOS_DISPLAY_ROTATION_VALUE
    
    // In Kconfig, these are booleans/ints.
    // We can map them directly if names align, or use conditional defines.
#endif

#if CONFIG_FLXOS_TOUCH_ENABLED
    #define FLXOS_TOUCH_ENABLED         true
#else
    #define FLXOS_TOUCH_ENABLED         false
#endif

// Add other mappings as needed by GuiTask.cpp
