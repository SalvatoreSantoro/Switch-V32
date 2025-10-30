#ifndef SV32_SDL_H
#define SV32_SDL_H

#include "SDL_events.h"
#include <stddef.h>
#include <stdint.h>

// SDL syscalls
#define SDL_INIT          2100
#define SDL_WRITE_PALETTE 2101
#define SDL_WRITE_FB      2102
#define SDL_PULL_EVENTS   2103
#define SDL_SHUTDOWN      2104

typedef enum {
    BUFF_SZ_POW_OF_2,
    NOT_POS_H,
    NOT_POS_W,
    BUFF_SZ_ZERO
} sdl_err;

sdl_err sdl_init(const char *win_name, int w, int h, uint32_t ev_buff_sz);

void sdl_write_palette(const uint32_t *p, uint32_t p_size);

int sdl_write_fb(const uint8_t *pix_idx);

// the application need to pass a pointer to SDL_Event array, pointer to head and value of tail
// the system call will load the events inside the array, updating the head accordingly
// (will treat the events array as a Circular buffer, using the ev_buff_sz given during sdl_init)
// returns the number of event written
unsigned int sdl_pull_events(SDL_Event *events, unsigned int *head, unsigned int tail);

void sdl_shutdown(void);

#endif
