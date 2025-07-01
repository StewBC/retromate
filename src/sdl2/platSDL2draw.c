/*
 *  platSDL2draw.c
 *  RetroMate
 *
 *  By S. Wessels, 2025.
 *  This is free and unencumbered software released into the public domain.
 *
 */

#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL.h>
#include <string.h>

#include "../global.h"

#include "platSDL2.h"

// The colors (C64 colors)
SDL_Color sdl2_palette[] = {
    {0x00, 0x00, 0x00, 0xff }, // COLOR_BLACK
    {0xFF, 0xFF, 0xFF, 0xff }, // COLOR_WHITE
    {0x88, 0x00, 0x00, 0xff }, // COLOR_RED
    {0xAA, 0xFF, 0xEE, 0xff }, // COLOR_CYAN
    {0xCC, 0x44, 0xCC, 0xff }, // COLOR_PURPLE
    {0x00, 0xCC, 0x55, 0xff }, // COLOR_GREEN
    {0x00, 0x00, 0xAA, 0xff }, // COLOR_BLUE
    {0xEE, 0xEE, 0x77, 0xff }, // COLOR_YELLOW
    {0xDD, 0x88, 0x55, 0xff }, // COLOR_ORANGE
    {0x66, 0x44, 0x00, 0xff }, // COLOR_BROWN
    {0xFF, 0x77, 0x77, 0xff }, // COLOR_LIGHTRED
    {0x33, 0x33, 0x33, 0xff }, // COLOR_GRAY1 (dark gray)
    {0x77, 0x77, 0x77, 0xff }, // COLOR_GRAY2 (medium gray)
    {0xAA, 0xFF, 0x66, 0xff }, // COLOR_LIGHTGREEN
    {0x00, 0x88, 0xFF, 0xff }, // COLOR_LIGHTBLUE
    {0xBB, 0xBB, 0xBB, 0xff }, // COLOR_GRAY3 (light gray)
};

// Map menu colors to the application colors
uint8_t plat_mc2pc[9] = {
    COLOR_BLUE,                 // MENU_COLOR_BACKGROUND
    COLOR_YELLOW,               // MENU_COLOR_FRAME
    COLOR_WHITE,                // MENU_COLOR_TITLE
    COLOR_CYAN,                 // MENU_COLOR_ITEM
    COLOR_GREEN,                // MENU_COLOR_CYCLE
    COLOR_LIGHTRED,             // MENU_COLOR_CALLBACK
    COLOR_PURPLE,               // MENU_COLOR_SUBMENU
    COLOR_WHITE,                // MENU_COLOR_SELECTED
    COLOR_RED,                  // MENU_COLOR_DISABLED
};

sdl_t sdl = {
    NULL,           // window;
    NULL,           // renderer;
    NULL,           // font;
    NULL,           // framebuffer;
    NULL,           // piece_texture;
    COLOR_WHITE,    // draw_color;
    COLOR_GREEN,    // text_bg_color
    -1,             // sockfd;
    {},             // recv_buf[1025];
    NULL            // receive_callback
};

#ifdef WIN32
static char *strndup(const char *s, size_t n) {
    size_t len = strnlen(s, n);
    char *new_str = (char *)malloc(len + 1);
    if (!new_str) {
        return NULL;
    }
    memcpy(new_str, s, len);
    new_str[len] = '\0';
    return new_str;
}
#endif

/*-----------------------------------------------------------------------*/
// The x, y are in Graphics space here
static void plat_draw_piece(uint8_t piece, int screen_x, int screen_y) {
    uint8_t index = fics_letter_to_piece(piece) - 1;
    if (index & PIECE_WHITE) {
        index &= ~PIECE_WHITE;
    } else {
        index += 6;
    }
    int sx = (index % 6) * 96; // Based on grid of assets/sdl2/pieces.png
    int sy = (index / 6) * 96;

    int ox = (SQUARE_DISPLAY_WIDTH - 96) / 2;
    int oy = (SQUARE_DISPLAY_HEIGHT - 96) / 2;

    SDL_Rect src = {sx, sy, 96, 96};
    SDL_Rect dst = {screen_x + ox, screen_y + oy, 96, 96};

    SDL_RenderCopy(sdl.renderer, sdl.piece_texture, &src, &dst);
}

/*-----------------------------------------------------------------------*/
void plat_draw_background() {
    // uint8_t t, l, b, r, tt, tb;
    // int8_t i, x, y, mw, mh;
    // mw = global.view.mc.x + global.view.mc.w;
    // mh = global.view.mc.y + global.view.mc.h;
    int8_t r = plat_core_get_status_x() - 1;
    int8_t x = global.view.mc.x + global.view.mc.w - r;

    // If the accoutrements are covered
    if (global.view.mc.x < 2) {
        plat_draw_rect(0, 0, 2, SCREEN_TEXT_HEIGHT, COLOR_GREEN);
    }

    if (x > 0) {
        // The menu covers part of the status area - clear it
        plat_draw_rect(r, global.view.mc.y, x, global.view.mc.h, COLOR_GREEN);
        plat_draw_log(&global.view.info_panel, plat_core_get_status_x(), 0, true);
    }

    // Always redraw these - because of the line around the board
    plat_draw_board_accoutrements();

    // Set up a square based clip top, left, bottom, right
    // t = 14;
    // for (i = 0, y = 0; y < 8; y++) {
    //     b = t + SQUARE_DISPLAY_HEIGHT;
    //     l = 2;
    //     r = 2 + SQUARE_TEXT_WIDTH;
    //     // find out what text rows these pieces now intercept
    //     tt = t >> 3;
    //     tb = b >> 3;
    //     for (x = 0; x < 8; x++) {
    //         if (l <= mw && r >= global.view.mc.x) {
    //             // tt <= mh && tb >= global.view.mc.y)  { // intersets
    //             plat_draw_square(i);
    //         }
    //         l += SQUARE_TEXT_WIDTH;
    //         r += SQUARE_TEXT_WIDTH;
    //         i++;
    //     }
    //     t += SQUARE_DISPLAY_HEIGHT;
    // }
    plat_draw_board();
}

/*-----------------------------------------------------------------------*/
// Draw the chess board and possibly clear the log section
void plat_draw_board_accoutrements() {
    // Column labels (a-h)
    for (int col = 0; col < 8; col++) {
        char label[2] = { 'a' + col, '\0' };
        SDL_Surface *surf = TTF_RenderText_Solid(sdl.font, label, sdl2_palette[COLOR_WHITE]);
        SDL_Texture *tex = SDL_CreateTextureFromSurface(sdl.renderer, surf);

        SDL_Rect dest = {
            .x = CHARACTER_WIDTH + 2 + col * SQUARE_DISPLAY_WIDTH + (SQUARE_DISPLAY_WIDTH - CHARACTER_WIDTH) / 2,
            .y = 8 * SQUARE_DISPLAY_HEIGHT,
            .w = CHARACTER_WIDTH,
            .h = surf->h
        };
        SDL_RenderCopy(sdl.renderer, tex, NULL, &dest);
        SDL_FreeSurface(surf);
        SDL_DestroyTexture(tex);
    }

    // Row labels (8-1)
    for (int row = 0; row < 8; row++) {
        char label[2] = { '8' - row, '\0' };
        SDL_Surface *surf = TTF_RenderText_Solid(sdl.font, label, sdl2_palette[COLOR_WHITE]);
        SDL_Texture *tex = SDL_CreateTextureFromSurface(sdl.renderer, surf);
        SDL_Rect dest = {
            .x = 0,
            .y = row * SQUARE_DISPLAY_HEIGHT + (SQUARE_DISPLAY_HEIGHT - CHARACTER_WIDTH) / 2,
            .w = CHARACTER_WIDTH,
            .h = surf->h
        };
        SDL_RenderCopy(sdl.renderer, tex, NULL, &dest);
        SDL_FreeSurface(surf);
        SDL_DestroyTexture(tex);
    }
}

/*-----------------------------------------------------------------------*/
void plat_draw_board() {
    plat_draw_board_accoutrements();
    for (int i = 0; i < 64; i++) {
        plat_draw_square(i);
    }
    global.view.refresh = 0;
}

/*-----------------------------------------------------------------------*/
void plat_draw_clear_input_line(bool active) {
    if (active) {
        plat_draw_rect(0, plat_core_get_rows() - 1, plat_core_get_cols(), 1, plat_mc2pc[MENU_COLOR_BACKGROUND]);
    } else {
        plat_draw_rect(0, plat_core_get_rows() - 1, plat_core_get_cols(), 1, COLOR_GREEN);
    }
}

/*-----------------------------------------------------------------------*/
void plat_draw_clear_statslog_area(uint8_t row) {
    plat_draw_rect(plat_core_get_status_x(), row, global.view.info_panel.cols, plat_core_get_rows() - row, COLOR_GREEN);
}

/*-----------------------------------------------------------------------*/
void plat_draw_clrscr() {
    SDL_Color clear_color = sdl2_palette[COLOR_GREEN];
    SDL_SetRenderDrawColor(sdl.renderer, clear_color.r, clear_color.g, clear_color.b, clear_color.a);
    SDL_RenderClear(sdl.renderer);
}

/*-----------------------------------------------------------------------*/
void plat_draw_highlight(uint8_t position, uint8_t color) {
    uint8_t y = (position / 8) * SQUARE_TEXT_HEIGHT;
    uint8_t x = (position & 7) * SQUARE_TEXT_WIDTH;
    color = color ? COLOR_BLUE : COLOR_GREEN;

    plat_draw_rect(x + 1, y, 1, SQUARE_TEXT_HEIGHT, color);
    plat_draw_rect(x + SQUARE_TEXT_WIDTH, y, 1, SQUARE_TEXT_HEIGHT, color);
}

/*-----------------------------------------------------------------------*/
void plat_draw_log(tLog *log, uint8_t x, uint8_t y, bool use_color) {
    int i;
    char *log_end = log->buffer + log->buffer_size;
    char *log_render = log->buffer;
    if (log->size >= log->rows) {
        log_render += log->cols * log->head;
    }
    log->modified = false;
    // This will only affect the status, the terminal is text
    sdl.draw_color = COLOR_BLACK;
    sdl.text_bg_color = COLOR_GREEN;

    for (i = 0; i < log->size; ++i) {
        plat_draw_text(x, y++, log_render, log->cols);
        log_render += log->cols;
        if (log_render >= log_end) {
            log_render = log->buffer;
        }
    }
}

/*-----------------------------------------------------------------------*/
// This operates in character units
void plat_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color) {
    SDL_SetRenderDrawColor(sdl.renderer, sdl2_palette[color].r, sdl2_palette[color].g, sdl2_palette[color].b, sdl2_palette[color].a);
    SDL_Rect r = {
        .x = x * CHARACTER_WIDTH,
        .y = y * CHARACTER_HEIGHT,
        .w = w * CHARACTER_WIDTH,
        .h = h * CHARACTER_HEIGHT
    };
    SDL_RenderFillRect(sdl.renderer, &r);
}

/*-----------------------------------------------------------------------*/
void plat_draw_set_color(uint8_t color) {
    sdl.draw_color = color;
}

/*-----------------------------------------------------------------------*/
void plat_draw_set_text_bg_color(uint8_t color) {
    sdl.text_bg_color = color;
}

/*-----------------------------------------------------------------------*/
void plat_draw_splash_screen() {
    uint8_t title1_len = strlen(global.text.title_line1);
    uint8_t title2_len = strlen(global.text.title_line2);
    // Clear the screen
    plat_draw_clrscr();

    // Show the welcome text
    plat_draw_set_color(COLOR_BLACK);
    plat_draw_text((SCREEN_TEXT_WIDTH - title1_len) / 2, SCREEN_TEXT_HEIGHT / 2 - 1, global.text.title_line1, title1_len);
    plat_draw_set_color(COLOR_WHITE);
    plat_draw_text((SCREEN_TEXT_WIDTH - title2_len) / 2, SCREEN_TEXT_HEIGHT / 2 + 1, global.text.title_line2, title2_len);

    plat_draw_piece('k', (SCREEN_DISPLAY_WIDTH - SQUARE_DISPLAY_WIDTH) / 2, (SCREEN_DISPLAY_HEIGHT / 2) - 3 * CHARACTER_HEIGHT - SQUARE_DISPLAY_HEIGHT);
    plat_draw_piece('K', (SCREEN_DISPLAY_WIDTH - SQUARE_DISPLAY_WIDTH) / 2, (SCREEN_DISPLAY_HEIGHT / 2) + 3 * CHARACTER_HEIGHT);

    plat_draw_update();

    // Hold the title screen
    plat_core_key_wait_any();

    // Clear the hires screen
    plat_draw_clrscr();
}

/*-----------------------------------------------------------------------*/
void plat_draw_square(uint8_t position) {
    uint8_t y = position / 8, x = position & 7;
    bool black_or_white = !((x & 1) ^ (y & 1));
    SDL_Color draw_color = sdl2_palette[black_or_white ? COLOR_WHITE : COLOR_GRAY2];
    SDL_SetRenderDrawColor(sdl.renderer, draw_color.r, draw_color.g, draw_color.b, draw_color.a);

    SDL_Rect rect1 = {
        .x = BOARD_START_X + x * SQUARE_DISPLAY_WIDTH,
        .y = y * SQUARE_DISPLAY_HEIGHT,
        .w = SQUARE_DISPLAY_WIDTH,
        .h = SQUARE_DISPLAY_HEIGHT
    };
    SDL_RenderFillRect(sdl.renderer, &rect1);

    uint8_t piece = global.state.chess_board[position++];
    if (piece != '-') {
        plat_draw_piece(piece, BOARD_START_X + x * SQUARE_DISPLAY_WIDTH, y * SQUARE_DISPLAY_HEIGHT);
    }
}

/*-----------------------------------------------------------------------*/
void plat_draw_text(uint8_t x, uint8_t y, const char *text, uint8_t len) {
    if (len) {
        char *ttf_text = strndup(text, len);
        if (!ttf_text) {
            return;
        }
        SDL_Surface *text_surface = TTF_RenderText_Shaded(sdl.font, ttf_text, sdl2_palette[sdl.draw_color], sdl2_palette[sdl.text_bg_color]);
        free(ttf_text);
        if (!text_surface) {
            return;
        }

        SDL_Texture *texture = SDL_CreateTextureFromSurface(sdl.renderer, text_surface);
        SDL_Rect dest = {
            .x = x * CHARACTER_WIDTH,
            .y = y * CHARACTER_HEIGHT,
            .w = text_surface->w,
            .h = text_surface->h
        };

        SDL_RenderCopy(sdl.renderer, texture, NULL, &dest);
        SDL_FreeSurface(text_surface);
        SDL_DestroyTexture(texture);
    }
}

/*-----------------------------------------------------------------------*/
void plat_draw_update(void) {
    // Remove sdl.framebuffer as the drawing target
    SDL_SetRenderTarget(sdl.renderer, NULL);
    // Copy the texture (sdl.framebuffer) to the window
    SDL_RenderCopy(sdl.renderer, sdl.framebuffer, NULL, NULL);
    // present the window
    SDL_RenderPresent(sdl.renderer);
    // Set sdl.framebuffer as the target for draw commands, again
    SDL_SetRenderTarget(sdl.renderer, sdl.framebuffer);
}