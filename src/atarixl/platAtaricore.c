/*
 *  platAtaricore.c
 *  RetroMate
 *
 *  By S. Wessels and O. Schmidt, 2025.
 *  This is free and unencumbered software released into the public domain.
 *
 */

#include <atari.h>
#include <conio.h>  // kbhit, cgetc
#include <stdlib.h> // exit
#include <string.h> // memcpy

#include "../global.h"

#include "platAtari.h"

#pragma code-name(push, "SHADOW_RAM2")

/*-----------------------------------------------------------------------*/
void plat_core_active_term(bool active) {
    if (active) {
        hires_done();
        clrscr();
        global.view.terminal_active = 1;
    } else {
        clrscr();
        hires_init();
        plat_draw_board();
        global.view.terminal_active = 0;
        plat_draw_log(&global.view.info_panel, plat_core_get_status_x(), 0, true);
    }
}

/*-----------------------------------------------------------------------*/
void plat_core_copy_ascii_to_display(void *dest, const void *src, size_t n) {
    memcpy(dest, src, n);
}

/*-----------------------------------------------------------------------*/
void plat_core_exit() {
    exit(1);
}

/*-----------------------------------------------------------------------*/
uint8_t plat_core_get_cols(void) {
    return SCREEN_TEXT_WIDTH;
}

/*-----------------------------------------------------------------------*/
uint8_t plat_core_get_rows(void) {
    return SCREEN_TEXT_HEIGHT;
}

/*-----------------------------------------------------------------------*/
uint8_t plat_core_get_status_x(void) {
    // The accoutrements (1..8) takes 1 char + the board + frame (2)
    return 3 + SQUARE_TEXT_WIDTH * 8;
}

/*-----------------------------------------------------------------------*/
void plat_core_init() {
    uint8_t i;
    // Assign a character that is in both hires and text, good as a cursor
    atari.CHAR_ROM = (char *)(*((char *)0x02F4) * 256);
    global.view.cursor_char[0] = 95;
    global.view.cursor_char[2] = 95;

    plat_draw_clrscr();
    hires_init();

    _setcolor(1, 0xc, 0xf);  // Pixel %1 color
    _setcolor(2, 0xc, 0x7);  // Pixel %0 color
    _setcolor(4, 0xC, 0x7);  // border color

    plat_draw_splash_screen();

    // Fill in the help text lengths
    for(i = 0; i < atari.help_text_num_lines; i++) {
        atari.help_text_len[i] = strlen(atari.help_text[i]);
    }
    plat_draw_board();

}

/*-----------------------------------------------------------------------*/
uint8_t plat_core_key_input(input_event_t *evt) {
    uint8_t k;

    evt->code = INPUT_NONE;

    if (!kbhit()) {
        return 0;
    }

    k = cgetc();
    evt->key_value = k;
    switch (k) {
        case 27:  // esc
            evt->code = INPUT_BACK;
            return 1;
        case 155:  // return
            evt->code = INPUT_SELECT;
            return 1;
        case 30:   // crsr left
            evt->code = INPUT_LEFT;
            return 1;
        case 31:  // crsr right
            evt->code = INPUT_RIGHT;
            return 1;
        case 28:  // crsr up
            evt->code = INPUT_UP;
            return 1;
        case 29:  // crsr down
            evt->code = INPUT_DOWN;
            return 1;
        case 127:   // tab
        case 20: // CTRL-T
            evt->code = INPUT_VIEW_TOGGLE;
            return 1;
        case 15: // CRTL+O
            evt->code = INPUT_VIEW_PAN_LEFT;
            return 1;
        case 16: // CTRL+P
            evt->code = INPUT_VIEW_PAN_RIGHT;
            return 1;
        case 19: // CTRL+S
            evt->code = INPUT_SAY;
            return 1;
        case 126: // DEL
        case 254: // DEL
            evt->code = INPUT_BACKSPACE;
            return 1;
        default:  // any other key
            evt->code = INPUT_KEY;
    }

    if (evt->code != INPUT_NONE) {
        return 1;
    }

    return 0;
}

/*-----------------------------------------------------------------------*/
void plat_core_key_wait_any() {
    while (kbhit()) {
        cgetc();
    }
    cgetc();
}

/*-----------------------------------------------------------------------*/
void plat_core_log_free_mem(char *mem) {
    UNUSED(mem);
}

#pragma code-name(pop)

/*-----------------------------------------------------------------------*/
char *plat_core_log_malloc(unsigned int size) {
    if (size == (80 * 23)) {
        return atari.terminal_log_buffer;
    }
    return atari.status_log_buffer;
}

/*-----------------------------------------------------------------------*/
uint8_t plat_core_mouse_to_cursor(void) {
    return MOUSE_HIT_NONE;
}

/*-----------------------------------------------------------------------*/
uint8_t plat_core_mouse_to_menu_item(void) {
    return MOUSE_HIT_NONE;
}

/*-----------------------------------------------------------------------*/
void plat_core_shutdown() {
}

