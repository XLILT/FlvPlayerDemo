#pragma once

#include <stdint.h>

extern int aac_init();
extern void aac_fina();

extern void on_aac_packet(char * buf, int64_t buf_size, int64_t dts);
