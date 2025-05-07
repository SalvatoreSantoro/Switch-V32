#ifndef _RV32_SDL_H
#define _RV32_SDL_H

#include <stddef.h>
#include <stdint.h>
void sdl_init(const char *win_name, int w, int h);

void sdl_write_palette(uint32_t *p, size_t p_size);

void sdl_write_fb(const uint8_t *pix_idx);

void sdl_shutdown(void);
#endif
