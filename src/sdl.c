#include "sdl.h"
#include "args.h"
#include "macros.h"
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


static SDL_Window *s_win;
static SDL_Renderer *s_ren;
static SDL_Texture *s_texture;
static uint32_t *s_palette = NULL;
static uint32_t *s_frame_buffer = NULL;
static int s_width;
static int s_height;

static size_t s_palette_size;
static size_t s_event_buff_size;
static size_t s_event_buff_size_mask = 0;

void sdl_init(const char *win_name, int w, int h, size_t ev_buff_sz) {
    // little optimization if size is power of 2
    if ((ev_buff_sz & (ev_buff_sz - 1)) != 0)
        SV32_CRASH("SDL Event buffer size must be power of 2\n");

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        SV32_CRASH(SDL_GetError());

    s_win = SDL_CreateWindow(win_name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, ctx.sdl_upscale * w,
                             ctx.sdl_upscale * h, 0);
    if (s_win == NULL) {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        goto sdl_quit;
    }

    s_ren = SDL_CreateRenderer(s_win, -1, SDL_RENDERER_ACCELERATED);
    if (s_ren == NULL) {
        fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
        goto win_destroy;
    }

    s_texture = SDL_CreateTexture(s_ren, SDL_PIXELFORMAT_BGRX32, SDL_TEXTUREACCESS_STREAMING, w, h);
    if (!s_texture) {
        fprintf(stderr, "SDL_CreateTexture Error: %s\n", SDL_GetError());
        goto ren_destroy;
    }

    s_frame_buffer = malloc(sizeof(*s_frame_buffer) * w * h);
    if (!s_frame_buffer) {
        fprintf(stderr, "SDL Frame buffer Error\n");
        goto ren_destroy;
    }

    // warp mouse for FPS
    if (SDL_SetRelativeMouseMode(SDL_TRUE) != 0) {
        fprintf(stderr, "SDL_SetRelativeMouseMode Error: %s\n", SDL_GetError());
        goto ren_destroy;
    }

    s_width = w;
    s_height = h;
    s_event_buff_size = ev_buff_sz;
    s_event_buff_size_mask = ev_buff_sz - 1;
    return;

ren_destroy:
    SDL_DestroyRenderer(s_ren);
win_destroy:
    SDL_DestroyWindow(s_win);
sdl_quit:
    SDL_Quit();
    exit(EXIT_FAILURE);
}

void sdl_write_palette(uint32_t *p, size_t p_size) {
    if (s_palette == NULL) {
        s_palette_size = p_size;
        s_palette = malloc(sizeof(*p) * p_size);
    }
    memcpy(s_palette, p, sizeof(*p) * s_palette_size);
}

void sdl_write_fb(const uint8_t *pix_idx) {
    for (int y = 0; y < s_height; y++) {
        for (int x = 0; x < s_width; x++) {
            int offset = (y * s_width) + x;
            s_frame_buffer[offset] = s_palette[pix_idx[offset]];
        }
    }

    SDL_UpdateTexture(s_texture, NULL, s_frame_buffer, s_width * 4);
    SDL_RenderClear(s_ren);
    SDL_RenderCopy(s_ren, s_texture, NULL, NULL);
    SDL_RenderPresent(s_ren);
}

void sdl_shutdown(void) {
    if (s_palette != NULL) {
        free(s_palette);
        s_palette = NULL;
    }
    if (s_frame_buffer != NULL) {
        free(s_frame_buffer);
        s_frame_buffer = NULL;
    }

    SDL_SetRelativeMouseMode(SDL_FALSE);
    SDL_DestroyRenderer(s_ren);
    SDL_DestroyWindow(s_win);
    SDL_Quit();

	exit(0);
}

// the application need to pass a pointer to SDL_Event array, pointer to head and value of tail
// the system call will load the events inside the array, updating the head accordingly
// returns the number of event written
int sdl_pull_events(SDL_Event *events, int *head, int tail) {
    SDL_Event e;
    int idx = *head;
    size_t i = 0;

    while ((SDL_PollEvent(&e)) && ((int) ((idx + 1) & s_event_buff_size_mask) != tail)) {
        if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP || e.type == SDL_MOUSEMOTION ||
            e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
            events[idx] = e;
            i += 1;
            idx = (idx + 1) & s_event_buff_size_mask;
        }
    }

    *head = idx;
    return i;
}
