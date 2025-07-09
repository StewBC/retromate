/*
 *  platC64draw.c
 *  RetroMate
 *
 *  By S. Wessels and O. Schmidt, 2025.
 *  This is free and unencumbered software released into the public domain.
 *
 */

#include <c64.h>
#include <conio.h>
#include <string.h>

#include "../global.h"

#include "platC64.h"

// Map menu colors to the application colors
uint8_t plat_mc2pc[9] = {
    COLOR_BLUE << 4   | COLOR_BLUE,     // MENU_COLOR_BACKGROUND
    COLOR_YELLOW << 4 | COLOR_YELLOW,   // MENU_COLOR_FRAME
    COLOR_WHITE << 4  | COLOR_WHITE,    // MENU_COLOR_TITLE
    COLOR_WHITE << 4  | COLOR_CYAN,     // MENU_COLOR_ITEM
    COLOR_WHITE << 4  | COLOR_GREEN,    // MENU_COLOR_CYCLE
    COLOR_WHITE << 4  | COLOR_LIGHTRED, // MENU_COLOR_CALLBACK
    COLOR_WHITE << 4  | COLOR_PURPLE,   // MENU_COLOR_SUBMENU
    COLOR_WHITE << 4  | COLOR_WHITE,    // MENU_COLOR_SELECTED
    COLOR_WHITE << 4  | COLOR_RED,      // MENU_COLOR_DISABLED
};

c64_t c64 = {
    {                                   // rop_line
        {0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F},
        {0x00, 0x40, 0x60, 0x70, 0x78, 0x7C, 0x7E}
    },
    {                                   // rop_color
        {0x55, 0x2A},
        {0xD5, 0xAA}
    },
    COLOR_WHITE << 4 | COLOR_GREEN,    // draw_colors
    SCREEN_TEXT_WIDTH,                 // terminal_display_width
};


/*-----------------------------------------------------------------------*/
// x in Character coords, y in Graphics coords
void plat_draw_char(char x, char y, unsigned rop, char c) {
    if (c >= 'A' && c <= 'Z') {
        c &= 0x7f;
    } else if (c >= 65 && c <= 90) {
        c -= 64;    // Lowercase to 1-26
    } else if(c >= 97 && c <= 123) {
        c -= 96;    // Lowercase ascii to 1-26
    }
    hires_draw(x, y, 1, 1, rop, CHARMAP_RAM + c * 8);
    hires_color(x, y, 1, 1, c64.draw_colors);
}

/*-----------------------------------------------------------------------*/
// Restore the background that a menu covered up
void plat_draw_background() {
    uint8_t t, l, b, r;
    int8_t i, x, y, mw, mh;
    mw = global.view.mc.x + global.view.mc.w;
    mh = global.view.mc.y + global.view.mc.h;
    r = plat_core_get_status_x() - 1;
    x = mw - r;

    // If the accoutrements are covered
    if (global.view.mc.x < 1) {
        hires_mask(0, 0, 1, SCREEN_TEXT_HEIGHT, ROP_BLACK);
        hires_color(0, 0, 1, SCREEN_TEXT_HEIGHT, COLOR_GREEN);
    }

    if (x > 0) {
        // The menu covers part of the status area - clear it
        hires_mask(r, global.view.mc.y, x, global.view.mc.h, ROP_BLACK);
        hires_color(r, global.view.mc.y, x, global.view.mc.h, COLOR_GREEN);
        plat_draw_log(&global.view.info_panel, plat_core_get_status_x(), 0, true);
    }

    // Always redraw these - because of the line around the board
    plat_draw_board_accoutrements();

    // Set up a square based clip top, left, bottom, right
    i = 0;
    t = 0;
    b = SQUARE_TEXT_HEIGHT;
    for (y = 0; y < 8; y++) {
        if(b >= global.view.mc.y) {
            if(t >= mh) {
                break;
            }
            l = 1;
            r = 1 + SQUARE_TEXT_WIDTH;
            for (x = 0; x < 8; x++) {
                if (l <= mw && r > global.view.mc.x) {
                    plat_draw_square(i+x);

                }
                l += SQUARE_TEXT_WIDTH;
                r += SQUARE_TEXT_WIDTH;
            }
        }
        t += SQUARE_TEXT_HEIGHT;
        b += SQUARE_TEXT_HEIGHT;
        i += 8;
    }
}

/*-----------------------------------------------------------------------*/
// Draw the chess board and possibly clear the log section
void plat_draw_board_accoutrements() {
    char i;

    c64.draw_colors = COLOR_GREEN;
    // Add the A..H and 1..8 tile-keys
    for (i = 0; i < 8; ++i) {
        plat_draw_char(2 + i * SQUARE_TEXT_WIDTH, 24, ROP_CPY, i + 'A');
        plat_draw_char(0, SCREEN_TEXT_HEIGHT - 3 - i * SQUARE_TEXT_HEIGHT, ROP_CPY, i + '1');
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
void plat_draw_clear_input_line(bool active) {
    if (global.view.terminal_active) {
        cclearxy(0, SCREEN_TEXT_HEIGHT - 1, c64.terminal_display_width);
    } else {
        hires_mask(0, SCREEN_TEXT_HEIGHT - 1, SCREEN_TEXT_WIDTH, 1, ROP_BLACK);
        hires_color(0, SCREEN_TEXT_HEIGHT - 1, SCREEN_TEXT_WIDTH, 1, active ? c64.draw_colors :  COLOR_GREEN);
    }
}

/*-----------------------------------------------------------------------*/
void plat_draw_clear_statslog_area(uint8_t row) {
    hires_mask(plat_core_get_status_x(), row,
               global.view.info_panel.cols, (SCREEN_TEXT_HEIGHT - row),
               ROP_BLACK);
}

/*-----------------------------------------------------------------------*/
void plat_draw_clrscr() {
    clrscr();
    hires_mask(0, 0, SCREEN_TEXT_WIDTH, SCREEN_TEXT_HEIGHT, ROP_BLACK);
    hires_color(0, 0, SCREEN_TEXT_WIDTH, SCREEN_TEXT_HEIGHT, (COLOR_WHITE << 4) | COLOR_GREEN);
}

/*-----------------------------------------------------------------------*/
void plat_draw_highlight(uint8_t position, uint8_t color) {
    uint8_t y = position / 8;
    uint8_t x = position & 7;

    if (color) {
        hires_color(1 + x * SQUARE_TEXT_WIDTH, y * SQUARE_TEXT_HEIGHT, 1, 3, COLOR_BLUE<<4|COLOR_BLUE);
        hires_color(SQUARE_TEXT_WIDTH + x * SQUARE_TEXT_WIDTH, y * SQUARE_TEXT_HEIGHT, 1, 3, COLOR_BLUE<<4|COLOR_BLUE);
    } else {
        hires_color(1 + x * SQUARE_TEXT_WIDTH, y * SQUARE_TEXT_HEIGHT, 1, 3, COLOR_GREEN<<4);
        hires_color(SQUARE_TEXT_WIDTH + x * SQUARE_TEXT_WIDTH, y * SQUARE_TEXT_HEIGHT, 1, 3, COLOR_GREEN<<4|COLOR_GREEN);
    }
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

    c64.draw_colors = COLOR_BLACK << 4 | COLOR_GREEN;

    if (width > c64.terminal_display_width) {
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
    hires_mask(x, y, w, h, ROP_BLACK);
    hires_color(x, y, w, h, color);
}

/*-----------------------------------------------------------------------*/
void plat_draw_set_color(uint8_t color) {
    c64.draw_colors = (color << 4) | (c64.draw_colors & 0x0f);
}

/*-----------------------------------------------------------------------*/
void plat_draw_set_text_bg_color(uint8_t color) {
    c64.draw_colors = (c64.draw_colors & 0xf0) | color;
}

/*-----------------------------------------------------------------------*/
void plat_draw_splash_screen() {
    uint8_t title1_len = strlen(global.text.title_line1);
    uint8_t title2_len = strlen(global.text.title_line2);

    // Show credits and wait for key press
    plat_draw_text((SCREEN_TEXT_WIDTH - title1_len) / 2, SCREEN_TEXT_HEIGHT / 2 - 1, global.text.title_line1, title1_len);
    hires_color((SCREEN_TEXT_WIDTH - title1_len) / 2, SCREEN_TEXT_HEIGHT / 2 - 1, title1_len, 1, COLOR_WHITE<<4|COLOR_GREEN);
    c64.draw_colors = COLOR_GREEN;
    plat_draw_text((SCREEN_TEXT_WIDTH - title2_len) / 2, SCREEN_TEXT_HEIGHT / 2 + 1, global.text.title_line2, title2_len);

    hires_draw(SCREEN_TEXT_WIDTH / 2 - 2, SCREEN_TEXT_HEIGHT / 2 - 6,
               SQUARE_TEXT_WIDTH, SQUARE_TEXT_HEIGHT, ROP_CPY,
               hires_pieces[KING - 1][SIDE_BLACK]);
    hires_color(SCREEN_TEXT_WIDTH / 2 - 2, SCREEN_TEXT_HEIGHT / 2 - 6, SQUARE_TEXT_WIDTH, SQUARE_TEXT_HEIGHT, COLOR_GREEN);
    hires_draw(SCREEN_TEXT_WIDTH / 2 - 2, SCREEN_TEXT_HEIGHT / 2 + 4,
               SQUARE_TEXT_WIDTH, SQUARE_TEXT_HEIGHT, ROP_CPY,
               hires_pieces[KING - 1][SIDE_WHITE]);
    hires_color(SCREEN_TEXT_WIDTH / 2 - 2, SCREEN_TEXT_HEIGHT / 2 + 6, SQUARE_TEXT_WIDTH, SQUARE_TEXT_HEIGHT, COLOR_WHITE << 4 | COLOR_GREEN);

    plat_core_key_wait_any();

    // Clear the screen again
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

    hires_draw(1 + x * SQUARE_TEXT_WIDTH, y * SQUARE_TEXT_HEIGHT,
               SQUARE_TEXT_WIDTH, SQUARE_TEXT_HEIGHT, rop,
               hires_pieces[(piece & 127) - 1][inv]);
    hires_color(1 + x * SQUARE_TEXT_WIDTH, y * SQUARE_TEXT_HEIGHT,
                SQUARE_TEXT_WIDTH, SQUARE_TEXT_HEIGHT, COLOR_WHITE<<4);
}

/*-----------------------------------------------------------------------*/
void plat_draw_text(uint8_t x, uint8_t y, const char *text, uint8_t len) {
    if (global.view.terminal_active) {
        gotoxy(x, y);
        while (len--) {
            // cputc(*text++);
            char c = *text++;
            if (c >= 65 && c < 91) {
                c |= 32;
            } else if (c >= 97 && c < 123) {
                c &= ~32;
            }
            cputc(c);
        }
    } else {
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
