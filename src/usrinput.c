/*
 *  usrinput.c
 *  RetroMate
 *
 *  By S. Wessels, 2025.
 *  This is free and unencumbered software released into the public domain.
 *
 */

#include <ctype.h>  // is*
#include <stdlib.h> // atoi

#include "global.h"

/*-----------------------------------------------------------------------*/
static uint8_t input_is_allowed(char c, uint8_t filter) {
    if (filter == FILTER_PRINTABLE) {
        return 0 != isprint(c);
    }
    if (filter == FILTER_ALNUM) {
        return 0 != isalnum(c);
    }
    if (filter == FILTER_NUM) {
        return (c >= '0' && c <= '9');
    }
    return 1;
}

/*-----------------------------------------------------------------------*/
void input_text(char *buffer, uint8_t buffer_len, uint8_t filter) {
    uint8_t index = 0;
    int8_t start_char;
    while (buffer[index] && index < buffer_len - 1) {
        index++;
    }
    buffer[index] = '\0';
    plat_draw_clear_input_line(1);

    while (1) {
        start_char = MAX(0, index - (plat_core_get_cols() - 2));

        // Set the input drawing color
        plat_draw_set_text_bg_color(plat_mc2pc[MENU_COLOR_BACKGROUND]);
        plat_draw_set_color(plat_mc2pc[MENU_COLOR_ITEM]);
        // Draw input line
        plat_draw_text(0, plat_core_get_rows() - 1, &buffer[start_char], index - start_char);
        // Draw Cursor
        plat_draw_text(index - start_char, plat_core_get_rows() - 1, global.view.cursor_char, 2);
        // Make sure it's on screen
        plat_draw_update();

        // Wait for a character
        while (!plat_core_key_input(&global.os.input_event)) {
            uint8_t game_state = global.app.state;
            if (global.view.terminal_active && global.view.terminal.modified) {
                plat_draw_log(&global.view.terminal, 0, 0, false);
                plat_draw_update();
            }
            if (plat_net_update() && game_state != global.app.state) {
                // Game went offline, so terminate this loop
                return;
            }

        }
        switch (global.os.input_event.code) {
            case INPUT_VIEW_TOGGLE:
                if (global.view.terminal_active) {
                    return;
                }
                break;

            case INPUT_QUIT:
            case INPUT_BACK:
                if (global.view.terminal_active) {
                    return;
                }
                // In normal edit, it erases whatever was there
                buffer[0] = '\0';
            // Fall Through

            case INPUT_SELECT:
                plat_draw_clear_input_line(0);
                if (global.view.terminal_active) {
                    log_add_line(&global.view.terminal, buffer, index);
                    plat_net_send(buffer);
                    buffer[0] = '\0';
                    plat_draw_clear_input_line(1);
                    index = 0;
                } else {
                    plat_draw_board_accoutrements();
                    return;
                }
                break;

            case INPUT_LEFT:
            case INPUT_BACKSPACE:
                if (index > 0) {
                    buffer[--index] = '\0';
                }
                break;

            case INPUT_VIEW_PAN_LEFT:
                global.view.pan_value--;
                global.view.terminal.modified = true;
                break;

            case INPUT_VIEW_PAN_RIGHT:
                global.view.pan_value++;
                global.view.terminal.modified = true;
                break;

            case INPUT_KEY: {
                    char c = global.os.input_event.key_value;
                    if (input_is_allowed(c, filter) && index < buffer_len - 1) {
                        buffer[index++] = c;
                        buffer[index] = '\0';
                    }
                }
                break;

            default:
                break;
        }
    }
}

//----------------------------------------------------------------------------
uint8_t input_text_callback(menu_t *m, void *data) {
    menu_item_t *item = (menu_item_t *)data;
    UNUSED(m);

    input_text(item->edit_target, item->edit_maxlen, item->filter);
    if (item->filter == FILTER_NUM && item->submenu) {
        *(int *)item->submenu = atoi(item->edit_target);
    }
    // Strings could change size, so the menu needs to be redrawn
    return MENU_DRAW_REDRAW;
}
