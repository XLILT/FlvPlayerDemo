#pragma once

#include <stdint.h>

extern int h264_init();
extern void h264_fina();

extern void on_264_packet(char * buf, int64_t buf_size, int64_t dts, int64_t pts);
