#pragma once

#include <SDL.h>
#include <stdint.h>

extern int pcm_render_init(int freq, SDL_AudioFormat format, uint8_t channels, uint16_t samples);
extern void on_pcm_packet(uint8_t * buf, int len, int64_t pts);
