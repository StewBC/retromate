/*
 *  platA2core.c
 *  RetroMate
 *
 *  By S. Wessels and O. Schmidt, 2025.
 *  This is free and unencumbered software released into the public domain.
 *
 */

#include <apple2.h>
#include <conio.h>  // kbhit, cgetc
#include <stdlib.h> // exit
#include <string.h> // memcpy

#include "../global.h"

#include "platA2.h"

/*-----------------------------------------------------------------------*/
apple2_t apple2 = {
    {                   // rop_line
        {0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F},
        {0x00, 0x40, 0x60, 0x70, 0x78, 0x7C, 0x7E}
    },
    {                   // rop_color - unused right now
        {0x55, 0x2A},
        {0xD5, 0xAA}
    },
    SCREEN_TEXT_WIDTH, // terminal_display_width
};

#pragma code-name(push, "LC")

/*-----------------------------------------------------------------------*/
void plat_core_active_term(bool active) {
    if (active) {
        hires_done();
        if (apple2.terminal_display_width == 80) {
            videomode(VIDEOMODE_80COL);
        }
        global.view.terminal_active = 1;
    } else {
        hires_init();
        global.view.terminal_active = 0;
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
    if (global.view.terminal_active) {
        return apple2.terminal_display_width;
    }
    return SCREEN_TEXT_WIDTH;
}

/*-----------------------------------------------------------------------*/
uint8_t plat_core_get_rows(void) {
    return SCREEN_TEXT_HEIGHT;
}

/*-----------------------------------------------------------------------*/
uint8_t plat_core_get_status_x(void) {
    // The accoutrements (1..8) takes 2 chars + the board + 1 extra
    return 3 + SQUARE_TEXT_WIDTH * 8;
}

/*-----------------------------------------------------------------------*/
void plat_core_init() {
    // Assign a character that is in both hires and text, good as a cursor
    global.view.cursor_char[0] = 95;
    global.view.cursor_char[2] = 95;
    plat_draw_splash_screen();
    plat_draw_board();

    if (videomode(VIDEOMODE_80COL) != -1) {
        apple2.terminal_display_width = 80;
    }
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
        case 13:  // return
            evt->code = INPUT_SELECT;
            return 1;
        case 8:   // crsr left
            evt->code = INPUT_LEFT;
            return 1;
        case 21:  // crsr right
            evt->code = INPUT_RIGHT;
            return 1;
        case 11:  // crsr up
            evt->code = INPUT_UP;
            return 1;
        case 10:  // crsr down
            evt->code = INPUT_DOWN;
            return 1;
        case 9:   // tab
        case 20: // CTRL-T
            evt->code = INPUT_VIEW_TOGGLE;
            return 1;
        case 15: // CRTL+O
            evt->code = INPUT_VIEW_PAN_LEFT;
            return 1;
        case 16: // CTRL+P
            evt->code = INPUT_VIEW_PAN_RIGHT;
            return 1;
        case 19:
            evt->code = INPUT_SAY;
            return 1;
        case 127: // DEL
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

/*-----------------------------------------------------------------------*/
char *plat_core_log_malloc(unsigned int size) {
    if (size == (80 * 23)) {
        return apple2.terminal_log_buffer;
    }
    return apple2.status_log_buffer;
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
    hires_done();
}

#pragma code-name(pop)
