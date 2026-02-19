#ifndef LV_CONF_H
#define LV_CONF_H

#ifndef __ASSEMBLER__
#include <SDL2/SDL.h>
#endif

#define LV_USE_LOG 1
#define LV_LOG_PRINTF 1
#define LV_COLOR_DEPTH 32
#define LV_TICK_CUSTOM 1

#ifndef __ASSEMBLER__
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (SDL_GetTicks())
#endif

#endif
