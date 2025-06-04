#ifndef _SRV32_SDL_H
#define _SRV32_SDL_H

#include <SDL_events.h>
#include <SDL_scancode.h>

// DOOM

// Input event types.
typedef enum {
    ev_keydown,
    ev_keyup,
    ev_mouse,
    ev_joystick
} evtype_t;

// WE'RE ASSUMING THAT INTs ARE THE SAME ON X86-64 and RV32, SHOUL BE OK BUT IT'S A LITTLE DANGEROUS

// Event structure.
typedef struct {
    evtype_t type;
    int data1; // keys / mouse/joystick buttons
    int data2; // mouse/joystick x move
    int data3; // mouse/joystick y move
} event_t;

//
// DOOM keyboard definition.
// This is the stuff configured by Setup.Exe.
// Most key data are simple ascii (uppercased).

#define KEY_RIGHTARROW SDL_SCANCODE_RIGHT
#define KEY_LEFTARROW  SDL_SCANCODE_LEFT
#define KEY_UPARROW    SDL_SCANCODE_UP
#define KEY_DOWNARROW  SDL_SCANCODE_DOWN
#define KEY_ESCAPE     SDL_SCANCODE_ESCAPE
#define KEY_ENTER      SDL_SCANCODE_RETURN
#define KEY_TAB        SDL_SCANCODE_TAB

#define KEY_F1  SDL_SCANCODE_F1
#define KEY_F2  SDL_SCANCODE_F2
#define KEY_F3  SDL_SCANCODE_F3
#define KEY_F4  SDL_SCANCODE_F4
#define KEY_F5  SDL_SCANCODE_F5
#define KEY_F6  SDL_SCANCODE_F6
#define KEY_F7  SDL_SCANCODE_F7
#define KEY_F8  SDL_SCANCODE_F8
#define KEY_F9  SDL_SCANCODE_F9
#define KEY_F10 SDL_SCANCODE_F10
#define KEY_F11 SDL_SCANCODE_F11
#define KEY_F12 SDL_SCANCODE_F12

#define KEY_BACKSPACE SDL_SCANCODE_BACKSPACE
#define KEY_PAUSE     SDL_SCANCODE_PAUSE

#define KEY_EQUALS SDL_SCANCODE_EQUALS
#define KEY_MINUS  SDL_SCANCODE_MINUS

#define KEY_RSHIFT SDL_SCANCODE_RSHIFT
#define KEY_RCTRL  SDL_SCANCODE_RCTRL
#define KEY_RALT   SDL_SCANCODE_RALT

#define KEY_LALT       SDL_SCANCODE_LALT

#endif
