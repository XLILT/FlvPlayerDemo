#pragma once

#include "stdint.h"

extern int yuv_render_init(int width, int height);

void render_yuv(uint8_t ** buf, int32_t * linesize, int64_t pts);
