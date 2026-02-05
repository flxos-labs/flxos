#include "Logger.hpp"
#include "lvgl.h"
#include "sdkconfig.h"
#include <SDL2/SDL.h>

namespace HAL {
void init_sdl();
void flush_cb(lv_display_t* display, const lv_area_t* area, uint8_t* px_map);
} // namespace HAL

int main(int argc, char* argv[]) {
	Log::info("Simulator", "Starting FlxOS Simulator...");

	lv_init();
	HAL::init_sdl();

	lv_display_t* disp = lv_display_create(320, 240);
	lv_display_set_flush_cb(disp, HAL::flush_cb);

	// Allocate draw buffers
	static lv_color_t buf1[320 * 240 / 10];
	lv_display_set_buffers(disp, buf1, nullptr, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

	Log::info("Simulator", "Running main loop...");
	bool running = true;
	while (running) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) running = false;
		}

		lv_timer_handler();
		SDL_Delay(5);
	}

	Log::info("Simulator", "Shutting down.");
	SDL_Quit();
	return 0;
}
