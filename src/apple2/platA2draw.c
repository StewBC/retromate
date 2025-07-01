/*
 *  platA2draw.c
 *  RetroMate
 *
 *  By S. Wessels and O. Schmidt, 2025.
 *  This is free and unencumbered software released into the public domain.
 *
 */

#include <conio.h>
#include <string.h>

#include "../global.h"

#include "platA2.h"

char rop_line[2][7] = {{0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F},
    {0x00, 0x40, 0x60, 0x70, 0x78, 0x7C, 0x7E}
};

char rop_color[2][2] = {{0x55, 0x2A}, {0xD5, 0xAA}};

// Map menu colors to the application colors
uint8_t plat_mc2pc[9] = {
    0,  // MENU_COLOR_BACKGROUND
    1,  // MENU_COLOR_FRAME
    0,  // MENU_COLOR_TITLE
    0,  // MENU_COLOR_ITEM
    0,  // MENU_COLOR_CYCLE
    0,  // MENU_COLOR_CALLBACK
    0,  // MENU_COLOR_SUBMENU
    0,  // MENU_COLOR_SELECTED
    0,  // MENU_COLOR_DISABLED
};

extern uint8_t terminal_display_width = SCREEN_TEXT_WIDTH;

#pragma code-name(push, "LC")

/*-----------------------------------------------------------------------*/
// x in Character coords, y in Graphics coords
static void plat_draw_char(char x, char y, unsigned rop, char c) {
    hires_draw(x, y, 1, 7, rop, hires_char_set[c - ' ']);
}

/*-----------------------------------------------------------------------*/
// Restore the background that a menu covered up
void plat_draw_background() {
    uint8_t t, l, b, r;//, tt, tb;
    int8_t i, x, y, mw, mh;
    mw = global.view.mc.x + global.view.mc.w;
    mh = global.view.mc.y + global.view.mc.h;
    r = plat_core_get_status_x() - 1;
    x = mw - r;

    // If the accoutrements are covered
    if (global.view.mc.x < 2) {
        hires_mask(0, 0, 2, SCREEN_DISPLAY_HEIGHT, ROP_BLACK);
    }

    if (x > 0) {
        // The menu covers part of the status area - clear it
        hires_mask(r, global.view.mc.y * CHARACTER_HEIGHT, x, global.view.mc.h  * CHARACTER_HEIGHT, ROP_BLACK);
        plat_draw_log(&global.view.info_panel, plat_core_get_status_x(), 0, true);
    }

    // Always redraw these - because of the line around the board
    plat_draw_board_accoutrements();

    // Set up a square based clip top, left, bottom, right
    t = 2;
    for (i = 0, y = 0; y < 8; y++) {
        b = t + SQUARE_DISPLAY_HEIGHT;
        l = 2;
        r = 2 + SQUARE_TEXT_WIDTH;
        // find out what text rows these pieces now intercept
        // tt = t >> 3;
        // tb = b >> 3;
        for (x = 0; x < 8; x++) {
            if (l <= mw && r >= global.view.mc.x) {
                // tt <= mh && tb >= global.view.mc.y)  { // intersets
                plat_draw_square(i);
            }
            l += SQUARE_TEXT_WIDTH;
            r += SQUARE_TEXT_WIDTH;
            i++;
        }
        t += SQUARE_DISPLAY_HEIGHT;
    }
}

/*-----------------------------------------------------------------------*/
// Draw the chess board and possibly clear the log section
void plat_draw_board_accoutrements() {
    char i;

    // Draw the board border
    hires_mask(1, 0, 1, 8 * SQUARE_DISPLAY_HEIGHT + 2 * 2, ROP_CONST(rop_line[1][2]));
    hires_mask(26, 0, 1, 8 * SQUARE_DISPLAY_HEIGHT + 2 * 2, ROP_CONST(rop_line[0][2]));
    hires_mask(2, 0, 8 * SQUARE_TEXT_WIDTH, 2, ROP_WHITE);
    hires_mask(2, 178, 8 * SQUARE_TEXT_WIDTH, 2, ROP_WHITE);

    // Add the A..H and 1..8 tile-keys
    for (i = 0; i < 8; ++i) {
        plat_draw_char(3 + i * SQUARE_TEXT_WIDTH, 184, ROP_CPY, i + 'A');
        plat_draw_char(0, SCREEN_DISPLAY_HEIGHT - 29 - i * SQUARE_DISPLAY_HEIGHT, ROP_CPY, i + '1');
    }
}

/*-----------------------------------------------------------------------*/
// Draw the chess board and possibly clear the log section
void plat_draw_board() {
    char i;
    plat_draw_board_accoutrements();
    for (i = 0; i < 64; ++i) {
        plat_draw_square(i);
    }
    global.view.refresh = 0;
}

/*-----------------------------------------------------------------------*/
void plat_draw_clear_input_line(bool) {
    if (global.view.terminal_active) {
        cclearxy(0, SCREEN_TEXT_HEIGHT - 1, terminal_display_width);
    } else {
        hires_mask(0, (SCREEN_TEXT_HEIGHT - 1) * CHARACTER_HEIGHT, SCREEN_TEXT_WIDTH, CHARACTER_HEIGHT, ROP_BLACK);
    }
}

/*-----------------------------------------------------------------------*/
void plat_draw_clear_statslog_area(uint8_t row) {
    hires_mask(plat_core_get_status_x(), CHARACTER_HEIGHT * row,
               global.view.info_panel.cols, CHARACTER_HEIGHT * (SCREEN_TEXT_HEIGHT - row),
               ROP_BLACK);
}

/*-----------------------------------------------------------------------*/
void plat_draw_clrscr() {
    clrscr();
    hires_mask(0, 0, SCREEN_TEXT_WIDTH, SCREEN_DISPLAY_HEIGHT, ROP_BLACK);
}

/*-----------------------------------------------------------------------*/
void plat_draw_highlight(uint8_t position, uint8_t color) {
    uint8_t y = position / 8;
    uint8_t x = position & 7;
    UNUSED(color);

    // if (color) {
    hires_mask(2 + x * SQUARE_TEXT_WIDTH, 2 + y * SQUARE_DISPLAY_HEIGHT + 4,
               1, SQUARE_DISPLAY_HEIGHT - 2 * 4, ROP_XOR(rop_line[0][4]));
    hires_mask(2 + x * SQUARE_TEXT_WIDTH + SQUARE_TEXT_WIDTH - 1, 2 + y * SQUARE_DISPLAY_HEIGHT + 4,
               1, SQUARE_DISPLAY_HEIGHT - 2 * 4, ROP_XOR(rop_line[1][4]));
    hires_mask(2 + x * SQUARE_TEXT_WIDTH, 2 + y * SQUARE_DISPLAY_HEIGHT,
               SQUARE_TEXT_WIDTH, 4, ROP_XOR(0x7F));
    hires_mask(2 + x * SQUARE_TEXT_WIDTH, 2 + y * SQUARE_DISPLAY_HEIGHT + SQUARE_DISPLAY_HEIGHT - 4,
               SQUARE_TEXT_WIDTH, 4, ROP_XOR(0x7F));
    // } else {
    //     uint8_t val = x & 1;
    //     hires_mask(2 + x * SQUARE_TEXT_WIDTH, 2 + y * SQUARE_DISPLAY_HEIGHT,
    //             1, SQUARE_DISPLAY_HEIGHT, ROP_AND(rop_color[color][!val]));
    //     hires_mask(2 + x * SQUARE_TEXT_WIDTH + 1, 2 + y * SQUARE_DISPLAY_HEIGHT,
    //             1, SQUARE_DISPLAY_HEIGHT, ROP_AND(rop_color[color][val]));
    //     hires_mask(2 + x * SQUARE_TEXT_WIDTH + 2, 2 + y * SQUARE_DISPLAY_HEIGHT,
    //             1, SQUARE_DISPLAY_HEIGHT, ROP_AND(rop_color[color][!val]));
    // }
}

/*-----------------------------------------------------------------------*/
void plat_draw_log(tLog *log, uint8_t x, uint8_t y, bool) {
    int i;
    char *log_end = log->buffer + log->buffer_size;
    char *log_render = log->buffer;
    uint8_t width = log->cols;

    if (log->size >= log->rows) {
        log_render += log->cols * log->head;
    }
    log->modified = false;

    if (width > terminal_display_width) {
        uint8_t shift = (global.view.pan_value & 0b11);
        if (shift == 0b11) {
            shift = 0;
            global.view.pan_value = 0;
        }
        width = SCREEN_TEXT_WIDTH;
        log_render += 20 * shift;
    }

    for (i = 0; i < log->size; ++i) {
        plat_draw_text(x, y++, log_render, width);
        log_render += log->cols;
        if (log_render >= log_end) {
            log_render = log->buffer + (log_render - log_end);
        }
    }
}

/*-----------------------------------------------------------------------*/
void plat_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color) {
    hires_mask(x, y * CHARACTER_HEIGHT, w, h * CHARACTER_HEIGHT, color ? ROP_WHITE : ROP_BLACK);
}

/*-----------------------------------------------------------------------*/
void plat_draw_set_color(uint8_t) {
}

/*-----------------------------------------------------------------------*/
void plat_draw_set_text_bg_color(uint8_t) {
}

/*-----------------------------------------------------------------------*/
void plat_draw_splash_screen() {
    // Show the hires screen
    hires_init();

    // Wait for key press
    plat_core_key_wait_any();

    // Clear the screen
    plat_draw_clrscr();
}

/*-----------------------------------------------------------------------*/
// Draw a tile with background and piece on it for positions 0..63
void plat_draw_square(uint8_t position) {
    unsigned rop;
    uint8_t inv;
    uint8_t y = position / 8;
    uint8_t x = position & 7;
    uint8_t piece = fics_letter_to_piece(global.state.chess_board[position]);
    bool black_or_white = !((x & 1) ^ (y & 1));

    if (piece) {
        rop = black_or_white ? ROP_INV : ROP_CPY;
        inv = black_or_white ^ !((piece & PIECE_WHITE) == 0);
    } else {
        rop = black_or_white ? ROP_WHITE : ROP_BLACK;
        inv = 0;
        piece = 1;
    }

    hires_draw(2 + x * SQUARE_TEXT_WIDTH, 2 + y * SQUARE_DISPLAY_HEIGHT,
               SQUARE_TEXT_WIDTH, SQUARE_DISPLAY_HEIGHT, rop,
               hires_pieces[(piece & 127) - 1][inv]);
}

/*-----------------------------------------------------------------------*/
void plat_draw_text(uint8_t x, uint8_t y, const char *text, uint8_t len) {
    if (global.view.terminal_active) {
        gotoxy(x, y);
        while (len--) {
            cputc(*text++);
        }
    } else {
        y *= CHARACTER_HEIGHT;
        while (len) {
            plat_draw_char(x++, y, ROP_CPY, *text);
            len--;
            text++;
        }
    }
}

/*-----------------------------------------------------------------------*/
void plat_draw_update() {
}

#pragma code-name(pop)
