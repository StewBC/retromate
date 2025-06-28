/*
 *  platSDL2.h
 *  RetroMate
 *
 *  By S. Wessels, 2025.
 *  This is free and unencumbered software released into the public domain.
 *
 */

#ifndef _PLATSDL2_H_
#define _PLATSDL2_H_

#ifdef _WIN32
#include <winsock2.h>
#endif

/*-----------------------------------------------------------------------*/
// These are text based coordinates
#define SCREEN_TEXT_WIDTH       80
#define SCREEN_TEXT_HEIGHT      25
#define SQUARE_TEXT_WIDTH       8
#define SQUARE_TEXT_HEIGHT      3

// These are graphical coordinates
#define CHARACTER_WIDTH         14
#define CHARACTER_HEIGHT        32
#define SCREEN_DISPLAY_WIDTH    (SCREEN_TEXT_WIDTH * CHARACTER_WIDTH)
#define SCREEN_DISPLAY_HEIGHT   (SCREEN_TEXT_HEIGHT * CHARACTER_HEIGHT)
#define SQUARE_DISPLAY_WIDTH    (SQUARE_TEXT_WIDTH * CHARACTER_WIDTH)
#define SQUARE_DISPLAY_HEIGHT   (SQUARE_TEXT_HEIGHT * CHARACTER_HEIGHT)
#define BOARD_DISPLAY_WIDTH     (SQUARE_DISPLAY_WIDTH * 8)
#define BOARD_DISPLAY_HEIGHT    (SQUARE_DISPLAY_HEIGHT * 8)
#define BOARD_START_X           (CHARACTER_WIDTH + 2)
#define BOARD_START_Y           0

// Using C64 colors in SDL as well
enum {
    COLOR_BLACK,
    COLOR_WHITE,
    COLOR_RED,
    COLOR_CYAN,
    COLOR_PURPLE,
    COLOR_GREEN,
    COLOR_BLUE,
    COLOR_YELLOW,
    COLOR_ORANGE,
    COLOR_BROWN,
    COLOR_LIGHTRED,
    COLOR_GRAY1,
    COLOR_GRAY2,
    COLOR_LIGHTGREEN,
    COLOR_LIGHTBLUE,
    COLOR_GRAY3,
};

typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Window SDL_Window;
typedef struct TTF_Font TTF_Font;
typedef struct SDL_Texture SDL_Texture;

typedef struct _sdl {
    SDL_Renderer *renderer;
    SDL_Window *window;
    TTF_Font *font;
    SDL_Texture *framebuffer;
    SDL_Texture *piece_texture;
    uint32_t draw_color;
    uint32_t text_bg_color;
#ifdef _WIN32
    SOCKET sockfd;
#else
    int sockfd;
#endif
    unsigned char recv_buf[1025];
    void (*receive_callback)(const unsigned char *data, int len);
} sdl_t;

extern sdl_t sdl;

#endif // _PLATSDL2_H_
