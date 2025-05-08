#include "sdl.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_scancode.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define UPS_W(x) (x << 2)
#define UPS_H(x) (x << 2)

static SDL_Window *__win;
static SDL_Renderer *__ren;
static SDL_Texture *__texture;
static uint32_t *__palette = NULL;
static uint32_t *__frame_buffer = NULL;
static int __width;
static int __height;

static size_t __palette_size;
static size_t __event_buff_size;
static size_t __event_buff_size_mask = 0;

void sdl_init(const char *win_name, int w, int h, size_t ev_buff_sz) {
    // little optimization if size is power of 2
    if ((ev_buff_sz & (ev_buff_sz - 1)) != 0) {
        fprintf(stderr, "SDL Event buffer size must be power of 2\n");
        exit(EXIT_FAILURE);
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    __win = SDL_CreateWindow(win_name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, UPS_W(w), UPS_H(h), 0);
    if (__win == NULL) {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        goto sdl_quit;
    }

    __ren = SDL_CreateRenderer(__win, -1, SDL_RENDERER_ACCELERATED);
    if (__ren == NULL) {
        fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
        goto win_destroy;
    }

    __texture = SDL_CreateTexture(__ren, SDL_PIXELFORMAT_BGRX32, SDL_TEXTUREACCESS_STREAMING, w, h);
    if (!__texture) {
        fprintf(stderr, "SDL_CreateTexture Error: %s\n", SDL_GetError());
        goto ren_destroy;
    }

    __frame_buffer = malloc(sizeof(*__frame_buffer) * w * h);
    if (!__frame_buffer) {
        fprintf(stderr, "SDL Frame buffer Error\n");
        goto ren_destroy;
    }

    __width = w;
    __height = h;
    __event_buff_size = ev_buff_sz;
    __event_buff_size_mask = ev_buff_sz - 1;
    return;

ren_destroy:
    SDL_DestroyRenderer(__ren);
win_destroy:
    SDL_DestroyWindow(__win);
sdl_quit:
    SDL_Quit();
    exit(EXIT_FAILURE);
}

void sdl_write_palette(uint32_t *p, size_t p_size) {
    if (__palette == NULL) {
        __palette_size = p_size;
        __palette = malloc(sizeof(*p) * p_size);
    }
    memcpy(__palette, p, sizeof(*p) * __palette_size);
}

void sdl_write_fb(const uint8_t *pix_idx) {
    for (int y = 0; y < __height; y++) {
        for (int x = 0; x < __width; x++) {
            int offset = (y * __width) + x;
            __frame_buffer[offset] = __palette[pix_idx[offset]];
        }
    }

    SDL_UpdateTexture(__texture, NULL, __frame_buffer, __width * 4);
    SDL_RenderClear(__ren);
    SDL_RenderCopy(__ren, __texture, NULL, NULL);
    SDL_RenderPresent(__ren);
}

void sdl_shutdown(void) {
    if (__palette != NULL) {
        free(__palette);
        __palette = NULL;
    }
    if (__frame_buffer != NULL) {
        free(__frame_buffer);
        __frame_buffer = NULL;
    }

    SDL_DestroyRenderer(__ren);
    SDL_DestroyWindow(__win);
    SDL_Quit();
}

// the application need to pass a pointer to SDL_Event array, pointer to head and value of tail
// the system call will load the events insid the array, updating the head accordingly
// returns the number of event written
int sdl_pull_events(event_t *events, int *head, int tail) {
    SDL_Event e;
    int idx = *head;
    size_t i = 0;
    evtype_t et;
    int data1 = 0;
    while ((SDL_PollEvent(&e)) && (((idx + 1) & __event_buff_size_mask) != tail)) {
        switch (e.type) {
        case SDL_KEYDOWN:
            et = ev_keydown;
            data1 = e.key.keysym.scancode;
            break;
        case SDL_KEYUP:
            et = ev_keyup;
            data1 = e.key.keysym.scancode;
            break;
        case SDL_MOUSEMOTION:
        case SDL_MOUSEBUTTONDOWN:
            et = ev_mouse;
            data1 = e.button.button;
            events[idx].data2 = e.motion.x;
            events[idx].data3 = e.motion.y;
            break;
        default:
            // if the event shouldn't be processed skip cycle
            continue;
        }
        events[idx].type = et;
        events[idx].data1 = e.key.keysym.scancode;
        i += 1;
        idx = (idx + 1) & __event_buff_size_mask;
    }

    *head = idx;
    return i;
}
