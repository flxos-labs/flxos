#include "Logger.hpp"
#include "lvgl.h"
#include <SDL2/SDL.h>

static SDL_Window* window = nullptr;
static SDL_Renderer* renderer = nullptr;
static SDL_Texture* texture = nullptr;

namespace HAL {

void init_sdl() {
	SDL_Init(SDL_INIT_VIDEO);
	window = SDL_CreateWindow("FlxOS Simulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 320, 240, 0);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 320, 240);
}

void flush_cb(lv_display_t* display, const lv_area_t* area, uint8_t* px_map) {
	// Basic SDL flush logic
	SDL_UpdateTexture(texture, NULL, px_map, 320 * 4);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
	lv_display_flush_ready(display);
}

} // namespace HAL
