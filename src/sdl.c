#include "memory.h"
#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define VID_BASE      (__vmem.m + 0x00100000)
#define VID_CTRL_BASE (VID_BASE + 0x00000)
#define VID_PAL_BASE  (VID_BASE + 0x10000)
#define VID_FB_BASE   (VID_BASE + 0x20000)
#define UART_BASE     (__vmem.m + 0x00103000)

#define WIDTH  320
#define HEIGHT 200

const SDL_Scancode map[] = {
    SDL_SCANCODE_LEFT,      // 0
    SDL_SCANCODE_RIGHT,     // 1
    SDL_SCANCODE_DOWN,      // 2
    SDL_SCANCODE_UP,        // 3
    SDL_SCANCODE_RSHIFT,    // 4
    SDL_SCANCODE_RCTRL,     // 5
    SDL_SCANCODE_RALT,      // 6
    SDL_SCANCODE_ESCAPE,    // 7
    SDL_SCANCODE_RETURN,    // 8
    SDL_SCANCODE_TAB,       // 9
    SDL_SCANCODE_BACKSPACE, // 10
    SDL_SCANCODE_PAUSE,     // 11
    SDL_SCANCODE_EQUALS,    // 12
    SDL_SCANCODE_MINUS,     // 13
    SDL_SCANCODE_F1,        // 14
    SDL_SCANCODE_F2,        // 15
    SDL_SCANCODE_F3,        // 16
    SDL_SCANCODE_F4,        // 17
    SDL_SCANCODE_F5,        // 18
    SDL_SCANCODE_F6,        // 19
    SDL_SCANCODE_F7,        // 20
    SDL_SCANCODE_F8,        // 21
    SDL_SCANCODE_F9,        // 22
    SDL_SCANCODE_F10,       // 23
    SDL_SCANCODE_F11,       // 24
    SDL_SCANCODE_F12        // 25
};

void *video_init(void *g) {
    static SDL_Window *window = NULL;
    static SDL_Renderer *renderer = NULL;
    static SDL_Texture *texture = NULL;
    static uint32_t *palette = (uint32_t *) VID_PAL_BASE;
    static uint8_t *framebuffer = (uint8_t *) VID_FB_BASE;
    static uint32_t *uart = (uint32_t *) UART_BASE;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return NULL;
    }

    window = SDL_CreateWindow("Pseudocolor Display", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH * 2,
                              HEIGHT * 2, SDL_WINDOW_SHOWN);
    if (!window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        return NULL;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    if (!renderer || !texture) {
        fprintf(stderr, "Renderer/Texture creation failed: %s\n", SDL_GetError());
        return NULL;
    }

    SDL_Event e;
    bool running = true;

    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
            } else if (e.type == SDL_KEYDOWN) {
                SDL_Scancode sc = e.key.keysym.scancode;
                for (int i = 0; i < sizeof(map) / sizeof(map[0]); ++i) {
                    if (sc == map[i]) {
                        *uart = (uint32_t) i;
                        printf("Key %d pressed (mapped scancode: %d)\n", uart, sc);
                        break;
                    }
                }
            }
        }
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                // You might want to set a flag here for graceful shutdown
            }
        }

        uint32_t *pixels = NULL;
        int pitch = 0;
        if (SDL_LockTexture(texture, NULL, (void **) &pixels, &pitch) != 0) {
            fprintf(stderr, "SDL_LockTexture Error: %s\n", SDL_GetError());
            return NULL;
        }

        for (int y = 0; y < HEIGHT; ++y) {
            for (int x = 0; x < WIDTH; ++x) {
                uint8_t index = framebuffer[y * WIDTH + x];
                pixels[y * (pitch / 4) + x] = palette[index];
            }
        }

        SDL_UnlockTexture(texture);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    return 0;
}
