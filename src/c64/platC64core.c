/*
 *  platC64core.c
 *  RetroMate
 *
 *  By S. Wessels and O. Schmidt, 2025.
 *  This is free and unencumbered software released into the public domain.
 *
 */

#include <c64.h>
#include <conio.h>  // kbhit, cgetc
#include <stdlib.h> // exit
#include <string.h> // memcpy

#include "../global.h"

#include "platC64.h"

#define MODKEY  (*(char*)653)
#define COMMODORE_KEY   2
#define CONTROL_KEY     4

char terminal_log_buffer[80 * 24];
char status_log_buffer[13 * 25];

/*-----------------------------------------------------------------------*/
void plat_core_active_term(bool active) {
    if (active) {
        plat_core_hires(false);
        c64.draw_colors = COLOR_GREEN; // COLOR_BLACK<<4|COLOR_GREEN
        global.view.terminal_active = 1;
    } else {
        plat_core_hires(true);
        global.view.terminal_active = 0;
    }
}

/*-----------------------------------------------------------------------*/
void plat_core_exit() {
    exit(1);
}

/*-----------------------------------------------------------------------*/
uint8_t plat_core_get_cols(void) {
    // if (global.view.terminal_active) {
    //     return terminal_display_width;
    // }
    return SCREEN_TEXT_WIDTH;
}

/*-----------------------------------------------------------------------*/
uint8_t plat_core_get_rows(void) {
    return SCREEN_TEXT_HEIGHT;
}

/*-----------------------------------------------------------------------*/
uint8_t plat_core_get_status_x(void) {
    // The accoutrements (1..8) takes 1 char + the board + 1 extra
    return 2 + SQUARE_TEXT_WIDTH * 8;
}

/*-----------------------------------------------------------------------*/
void plat_core_hires(bool on) {
    if (on) {
        // Select the bank
        CIA2.pra = (CIA2.pra & 0xFC) | (3 - (VIC_BASE_RAM >> 14));
        // Set the location of the Screen (color) and bitmap (0 or 8192 only)
        VIC.addr = ((((int)(SCREEN_RAM - VIC_BASE_RAM) / 0x0400) << 4) | (BITMAP_OFFSET / 0X400));

        // Turn on HiRes mode
        VIC.ctrl1 = (VIC.ctrl1 & 0xBF) | 0x20;
        VIC.ctrl2 = (VIC.ctrl2 & 0xEF);
    } else {
        // Turn HiRes off
        // Select the bank
        CIA2.pra = 151;
        // Set the location of the Screen (color) and bitmap (0 or 8192 only)
        VIC.addr = 23;

        // Turn on HiRes mode
        VIC.ctrl1 = 27;
        VIC.ctrl2 = 200;
    }
}

/*-----------------------------------------------------------------------*/
void plat_core_init() {
    // Assign a character that is in both hires and text, good as a cursor
    global.view.cursor_char[0] = 160;

    plat_draw_clrscr();
    plat_core_hires(true);

    // Turn off the timer
    CIA1.cra &= 0xfe;
    // CHAREN - Map characters into CPU
    *(char *)0x01 &= 0xfb;
    // Copy the standard font to where the char font will live
    memcpy((char *)CHARMAP_RAM, (char *)CHARMAP_ROM + 256 * 8, 256 * 8);
    // Unmap character rom from CPU
    *(char *)0x01 |= 0x04;
    // Turn timer back on
    CIA1.cra |= 0x01;

    plat_draw_splash_screen();
    plat_draw_board();
}

/*-----------------------------------------------------------------------*/
uint8_t plat_core_key_input(input_event_t *evt) {
    uint8_t k, mod;

    evt->code = INPUT_NONE;

    if (!kbhit()) {
        return 0;
    }

    k = cgetc();
    mod = MODKEY;
    evt->key_value = k;
    switch (k) {
        case 3:  // esc
            evt->code = INPUT_BACK;
            return 1;
        case 13:  // return
            evt->code = INPUT_SELECT;
            return 1;
        case 157:   // crsr left
            evt->code = INPUT_LEFT;
            return 1;
        case 29:  // crsr right
            evt->code = INPUT_RIGHT;
            return 1;
        case 145:  // crsr up
            evt->code = INPUT_UP;
            return 1;
        case 17:  // crsr down
            evt->code = INPUT_DOWN;
            return 1;
        case 9:   // tab
        case 84: // CTRL-T
            if (mod & CONTROL_KEY) {
                evt->code = INPUT_VIEW_TOGGLE;
                return 1;
            }
            goto justakey;
        case 79: // CRTL+O
            if (mod & CONTROL_KEY) {
                evt->code = INPUT_VIEW_PAN_LEFT;
                return 1;
            }
            goto justakey;
        case 80: // CTRL+P
            if (mod & CONTROL_KEY) {
                evt->code = INPUT_VIEW_PAN_RIGHT;
                return 1;
            }
            goto justakey;
        case 83:
            if (mod & CONTROL_KEY) {
                evt->code = INPUT_SAY;
                return 1;
            }
            goto justakey;
        case 20: // DEL
            evt->code = INPUT_BACKSPACE;
            return 1;
        default:  // any other key
justakey:
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
    if (size == (80 * 24)) {
        return terminal_log_buffer;
    }
    return status_log_buffer;
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
    __asm__("jmp 64738");
}
