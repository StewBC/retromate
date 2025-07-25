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

/*-----------------------------------------------------------------------*/
void plat_core_active_term(bool active) {
    if (active) {
        plat_core_hires(false);
        VIC.bordercolor = COLOR_BLACK;
        c64.draw_colors = COLOR_GREEN; // COLOR_BLACK<<4|COLOR_GREEN
        global.view.terminal_active = 1;
    } else {
        VIC.bordercolor = COLOR_GREEN;
        plat_core_hires(true);
        global.view.terminal_active = 0;
    }
}

/*-----------------------------------------------------------------------*/
void plat_core_copy_ascii_to_display(void *dest, const void *src, size_t n) {
    char *from = (char*)src;
    char *to = (char*)dest;
    while (n--) {
        char c = *from++;
        if (c >= 65 && c < 91) {    // 'A-Z' to PETSCII
            c |= 0x80;
        }
        *to++ = c;
    }
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
    uint8_t i, j;

    // Assign character that are good as a cursors
    global.view.cursor_char[0] = 248;   // HiRes
    global.view.cursor_char[2] = 162;   // Text (Terminal)

    plat_draw_clrscr();
    plat_core_active_term(false);

    // Turn off the timer
    CIA1.cra &= 0xfe;
    // CHAREN - Map characters into CPU
    *(char *)0x01 &= 0xfb;
    // Copy the shifted font to where the char font will live
    memcpy((char *)CHARMAP_RAM, (char *)CHARMAP_ROM + 256 * 8, 256 * 8);
    // Unmap character rom from CPU
    *(char *)0x01 |= 0x04;
    // Turn timer back on
    CIA1.cra |= 0x01;

    // Fill in the help text lengths
    for(i = 0; i < 2; i++) {
        for(j = 0; j < c64.help_text_num_lines[i]; j++) {
            c64.help_text_len[i][j] = strlen(c64.help_text[i][j]);
        }
    }
    VIC.bgcolor0 = COLOR_BLACK;
    *CHARCOLOR = COLOR_WHITE;
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

    mod = MODKEY;
    k = cgetc();
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
        case 20: // CTRL-T
            if (mod & CONTROL_KEY) {
                evt->code = INPUT_VIEW_TOGGLE;
                return 1;
            }
            evt->code = INPUT_BACKSPACE;
            return 1;
        case 15: // CRTL+O
            if (mod & CONTROL_KEY) {
                evt->code = INPUT_VIEW_PAN_LEFT;
                return 1;
            }
            goto justakey;
        case 16: // CTRL+P
            if (mod & CONTROL_KEY) {
                evt->code = INPUT_VIEW_PAN_RIGHT;
                return 1;
            }
            goto justakey;
        case 19: // CTRL+S
            if (mod & CONTROL_KEY) {
                evt->code = INPUT_SAY;
                return 1;
            }
            goto justakey;
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
        return c64.terminal_log_buffer;
    }
    return c64.status_log_buffer;
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
