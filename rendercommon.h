#pragma once

#include <stdint.h>
#include <chrono>

struct SDL_Window;

const int64_t INIT_PST_DRIFT = 0x7FFFFFFFFFFFFFFF;

extern SDL_Window * screen;
extern int64_t audio_pts_drift;

extern int render_init();
extern void render_fina();
extern void render_event_loop();

int64_t get_steady_time_ms();
