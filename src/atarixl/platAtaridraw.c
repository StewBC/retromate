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

#include "platAtari.h"

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

char *help_text[] = {
    "            RetroMate",
    "",
    "Chess board View:",
    "ESC       - show/hide menu",
    "CTRL+t/TAB- switch to terminal view",
    "CTRL+s    - \"say\" to opponent",
    "crsr/wasd - move cursor",
    "enter     - select/deselect square",
    "",
    "Terminal View:",
    "CTRL+t/TAB- switch to board view",
    "CTRL+p    - show text to the right",
    "CTRL+o    - show text to the left",
    "Type commands to execute them",
    "",
    "By S. Wessels and O. Schmidt, 2025"
};

uint8_t help_text_len[] = {
    0, // "            RetroMate",
    0, // "",
    0, // "Chess board View:",
    0, // "ESC       - show/hide menu",
    0, // "CTRL+t/TAB- switch to terminal view",
    0, // "CTRL+s    - \"say\" to opponent",
    0, // "crsr/wasd - move cursor",
    0, // "enter     - select/deselect square",
    0, // "",
    0, // "Terminal View:",
    0, // "CTRL+t/TAB- switch to board view",
    0, // "CTRL+p    - show text to the right",
    0, // "CTRL+o    - show text to the left",
    0, // "Type commands to execute them",
    0, // "",
    0, // "By S. Wessels and O. Schmidt, 2025"
};

/*-----------------------------------------------------------------------*/
atari_t atari = {
    {                   // rop_line
        {0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F},
        {0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE}
    },
    {
        {0x55, 0x2A},
        {0xD5, 0xAA}
    },
    help_text,
    help_text_len,
    AS(help_text_len),
    SCREEN_TEXT_WIDTH, // terminal_display_width
    0                  // CHAR_ROM
};

#pragma code-name(push, "SHADOW_RAM2")

/*-----------------------------------------------------------------------*/
// x in Character coords, y in Graphics coords
static void plat_draw_char(char x, char y, unsigned rop, char c) {
    if (c < 'a' || c > 'z') {
        c -= 32;
    }
    hires_draw(x, y, 1, 8, rop, atari.CHAR_ROM + c * 8);
}

/*-----------------------------------------------------------------------*/
// Restore the background that a menu covered up
void plat_draw_background() {
    uint8_t t, l, b, r, mt, mb, mw;
    int8_t i, x, y;
    mw = global.view.mc.x + global.view.mc.w;
    mt = global.view.mc.y << 3;
    mb = mt + (global.view.mc.h << 3);
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

    i = 0;                  // row of tile to potentially draw
    t = 2;                  // This is in pixels
    b = 2 + SQUARE_DISPLAY_HEIGHT;
    for(y = 0; y < 8; y++) {
        if(b >= mt) {       // Past top?
            if(t > mb) {    // Past bottom?
                break;      // past bottom done
            }
            l = 2;          // This in character columns
            r = 2 + SQUARE_TEXT_WIDTH;
            for(x = 0; x < 8; x++) {
                            // Intersect?
                if(l < mw && r > global.view.mc.x) {
                    plat_draw_square(i+x);

                }
                l += SQUARE_TEXT_WIDTH;
                r += SQUARE_TEXT_WIDTH;
            }
        }
        t += SQUARE_DISPLAY_HEIGHT;
        b += SQUARE_DISPLAY_HEIGHT;
        i += 8;
    }
}

/*-----------------------------------------------------------------------*/
// Draw the chess board and possibly clear the log section
void plat_draw_board_accoutrements() {
    char i;

    // Draw the board border
    hires_mask(1, 0, 1, 8 * SQUARE_DISPLAY_HEIGHT + 2 * 2, ROP_CONST(atari.rop_line[0][2]));
    hires_mask(26, 0, 1, 8 * SQUARE_DISPLAY_HEIGHT + 2 * 2, ROP_CONST(atari.rop_line[1][2]));
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
        cclearxy(0, SCREEN_TEXT_HEIGHT - 1, atari.terminal_display_width);
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
    hires_mask(0, 0, SCREEN_TEXT_WIDTH, SCREEN_DISPLAY_HEIGHT, ROP_BLACK);
}

/*-----------------------------------------------------------------------*/
void plat_draw_highlight(uint8_t position, uint8_t color) {
    uint8_t y = position / 8;
    uint8_t x = position & 7;
    bool black_or_white = !((x & 1) ^ (y & 1));

    if (color) {
        hires_mask(2 + x * SQUARE_TEXT_WIDTH, 2 + y * SQUARE_DISPLAY_HEIGHT + SQUARE_DISPLAY_HEIGHT/3, SQUARE_TEXT_WIDTH, SQUARE_DISPLAY_HEIGHT/2-2, ROP_INV);
    } else {
        hires_mask(2 + x * SQUARE_TEXT_WIDTH, 2 + y * SQUARE_DISPLAY_HEIGHT, 1, SQUARE_DISPLAY_HEIGHT, ROP_CONST(atari.rop_line[black_or_white ? 1 : 0][4]));
        hires_mask(2 + x * SQUARE_TEXT_WIDTH + SQUARE_TEXT_WIDTH - 1, 2 + y * SQUARE_DISPLAY_HEIGHT, 1, SQUARE_DISPLAY_HEIGHT, ROP_CONST(atari.rop_line[black_or_white ? 0 : 1][4]));
    }
}

#pragma code-name(pop)

#pragma code-name(push, "SHADOW_RAM")

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

    if (width > atari.terminal_display_width) {
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
    uint8_t title1_len = strlen(global.text.title_line1);
    uint8_t title2_len = strlen(global.text.title_line2);

    // Clear the screen
    plat_draw_clrscr();

    // Show the hires screen
    hires_init();

    // Show credits and wait for key press
    plat_draw_text((SCREEN_TEXT_WIDTH - title1_len) / 2, SCREEN_TEXT_HEIGHT / 2 - 1, global.text.title_line1, title1_len);
    plat_draw_text((SCREEN_TEXT_WIDTH - title2_len) / 2, SCREEN_TEXT_HEIGHT / 2 + 1, global.text.title_line2, title2_len);

    hires_draw(SCREEN_TEXT_WIDTH / 2 - 2, SCREEN_DISPLAY_HEIGHT / 2 - 50 - SQUARE_DISPLAY_HEIGHT / 2,
               SQUARE_TEXT_WIDTH, SQUARE_DISPLAY_HEIGHT, ROP_CPY,
               hires_pieces[KING - 1][SIDE_BLACK]);
    hires_draw(SCREEN_TEXT_WIDTH / 2 - 2, SCREEN_DISPLAY_HEIGHT / 2 + 50 - SQUARE_DISPLAY_HEIGHT / 2,
               SQUARE_TEXT_WIDTH, SQUARE_DISPLAY_HEIGHT, ROP_CPY,
               hires_pieces[KING - 1][SIDE_WHITE]);

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

#pragma code-name(pop)

/*-----------------------------------------------------------------------*/
uint8_t plat_draw_ui_help_callback(menu_t *m, void *data) {
    uint8_t line, h, s;
    UNUSED(m);
    UNUSED(data);

    h = atari.help_text_num_lines + 2;
    s = (SCREEN_TEXT_HEIGHT - h) / 2;

    // Clear background
    plat_draw_rect(2, s, 37, h, 0);


    // Draw a frame
    hires_mask(1, s*CHARACTER_HEIGHT, 1, h*CHARACTER_HEIGHT+2, ROP_CONST(atari.rop_line[0][2]));
    hires_mask(39, s*CHARACTER_HEIGHT, 1, h*CHARACTER_HEIGHT+2, ROP_CONST(atari.rop_line[1][2]));
    hires_mask(2, s*CHARACTER_HEIGHT, 37, 2, ROP_WHITE);
    hires_mask(2, (s+h)*CHARACTER_HEIGHT, 37, 2, ROP_WHITE);
    s++;

    // Show the text
    for(line = 0; line < atari.help_text_num_lines; line++) {
        plat_draw_text(3, s + line, atari.help_text[line], atari.help_text_len[line]);
    }

    plat_core_key_wait_any();
    plat_draw_clrscr();
    plat_draw_board();

    return MENU_DRAW_REDRAW;
}

/*-----------------------------------------------------------------------*/
void plat_draw_update() {
}

