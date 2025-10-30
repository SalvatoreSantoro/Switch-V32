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

static uint32_t s_palette_size;
static uint32_t s_event_buff_size;
static uint32_t s_event_buff_size_mask = 0;

sdl_err sdl_init(const char *win_name, int w, int h, uint32_t ev_buff_sz) {
    // little optimization if size is power of 2
    if (ev_buff_sz == 0)
        return BUFF_SZ_ZERO; // new error code, or handle as you prefer

    if ((ev_buff_sz & (ev_buff_sz - 1)) != 0)
        return BUFF_SZ_POW_OF_2;

    if (w <= 0)
        return NOT_POS_W;

    if (h <= 0)
        return NOT_POS_H;

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        SV32_CRASH(SDL_GetError());

    s_win = SDL_CreateWindow(win_name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, (int) ctx.sdl_upscale * w,
                             (int) ctx.sdl_upscale * h, 0);
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

    s_frame_buffer = malloc(sizeof(*s_frame_buffer) * (size_t) w * (size_t) h);
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
    return 0;

ren_destroy:
    SDL_DestroyRenderer(s_ren);
win_destroy:
    SDL_DestroyWindow(s_win);
sdl_quit:
    SDL_Quit();
    exit(EXIT_FAILURE);
}


void sdl_write_palette(const uint32_t *p, uint32_t p_size) {
    if (!p || p_size == 0) {
        fprintf(stderr, "sdl_write_palette: invalid palette pointer or size\n");
        return;
    }

    if (s_palette == NULL) {
        s_palette = malloc(sizeof(*s_palette) * (size_t)p_size);
        if (s_palette == NULL) {
            SV32_CRASH("OOM in SDL");
        }
        s_palette_size = p_size;

    } else if (p_size != s_palette_size) {
        // resize palette if needed
        uint32_t *newp = realloc(s_palette, sizeof(*s_palette) * (size_t)p_size);
        if (!newp) {
            SV32_CRASH("OOM in SDL (realloc)");
        }
        s_palette = newp;
        s_palette_size = p_size;
    }
    memcpy(s_palette, p, sizeof(*p) * (size_t)s_palette_size);
}


int sdl_write_fb(const uint8_t *pix_idx) {
    if (!pix_idx)
        return -1;

    // sdl_init wasn't called
    if (!s_palette)
        return -1;

    if (!s_frame_buffer)
        return -1;

    if (!s_texture)
        return -1;

    if (s_width <= 0 || s_height <= 0)
        return -1;

    if (s_palette == NULL)
        return -1;

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
    return 0;
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
unsigned int sdl_pull_events(SDL_Event *events, unsigned int *head, unsigned int tail) {
    SDL_Event e;
    unsigned int idx = *head;
    unsigned int i = 0;

    while ((SDL_PollEvent(&e)) && (((idx + 1) & s_event_buff_size_mask) != tail)) {
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
