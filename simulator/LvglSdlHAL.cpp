#include "../components/lvgl/src/lv_conf_internal.h"
#include "Logger.hpp"
#include "lvgl.h"
#include <SDL2/SDL.h>

static SDL_Window* window = nullptr;
static SDL_Renderer* renderer = nullptr;
static SDL_Texture* texture = nullptr;

namespace HAL {

bool init_sdl() {
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		Log::error("Simulator", "SDL_Init Error: %s", SDL_GetError());
		return false;
	}
	window = SDL_CreateWindow("FlxOS Simulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 320, 240, 0);
	if (window == nullptr) {
		Log::error("Simulator", "SDL_CreateWindow Error: %s", SDL_GetError());
		return false;
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	if (renderer == nullptr) {
		Log::error("Simulator", "SDL_CreateRenderer Error: %s", SDL_GetError());
		return false;
	}

	uint32_t format = SDL_PIXELFORMAT_ARGB8888;
#if LV_COLOR_DEPTH == 16
	format = SDL_PIXELFORMAT_RGB565;
#endif

	texture = SDL_CreateTexture(renderer, format, SDL_TEXTUREACCESS_STREAMING, 320, 240);
	if (texture == nullptr) {
		Log::error("Simulator", "SDL_CreateTexture Error: %s", SDL_GetError());
		return false;
	}

	return true;
}

void flush_cb(lv_display_t* display, const lv_area_t* area, uint8_t* px_map) {
	// Basic SDL flush logic
	SDL_Rect rect;
	rect.x = area->x1;
	rect.y = area->y1;
	rect.w = area->x2 - area->x1 + 1;
	rect.h = area->y2 - area->y1 + 1;

#if LV_COLOR_DEPTH == 16
	// For 16-bit color (RGB565), stride is width * 2 bytes
	SDL_UpdateTexture(texture, &rect, px_map, rect.w * 2);
#else
	// For 32-bit color (ARGB8888), stride is width * 4 bytes
	SDL_UpdateTexture(texture, &rect, px_map, rect.w * 4);
#endif

	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
	lv_display_flush_ready(display);
}

} // namespace HAL
