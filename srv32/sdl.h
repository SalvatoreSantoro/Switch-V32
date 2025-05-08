#ifndef _RV32_SDL_H
#define _RV32_SDL_H

#include "../common/sdl_syscalls.h"
#include <stddef.h>
#include <stdint.h>


void sdl_init(const char *win_name, int w, int h, size_t ev_buff_sz);

void sdl_write_palette(uint32_t *p, size_t p_size);

void sdl_write_fb(const uint8_t *pix_idx);

// the application need to pass a pointer to SDL_Event array, pointer to head and value of tail
// the system call will load the events inside the array, updating the head accordingly 
// (will treat the events array as a Circular buffer, using the ev_buff_sz given during sdl_init)
// returns the number of event written
int sdl_pull_events(event_t* events, int *head, int tail);

void sdl_shutdown(void);

#endif
