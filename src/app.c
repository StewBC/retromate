/*
 *  app.c
 *  RetroMate
 *
 *  By S. Wessels, 2025.
 *  This is free and unencumbered software released into the public domain.
 *
 */

#include <ctype.h>
#include <string.h>

#include "global.h"

#define COMMAND_LENGTH 128

/*-----------------------------------------------------------------------*/
static void app_terminal() {
    char command[COMMAND_LENGTH] = {0};
    // Go to terminal mode
    plat_core_active_term(1);
    // Force a draw
    global.view.terminal.modified = true;
    // Run the input while updating the terminal and net
    input_text(command, COMMAND_LENGTH, FILTER_ALLOW_ALL);
    // Done with terminal mode
    plat_core_active_term(0);
    // Redraw the board
    global.view.refresh = true;
    // Force a redraw of the status log
    global.view.info_panel.modified = true;
    // If the menu should be visible, make sure to draw it again
    if (!(global.view.mc.df & MENU_DRAW_HIDDEN)) {
        global.view.mc.df = MENU_DRAW;
    }
}

/*-----------------------------------------------------------------------*/
void app_draw_update() {
    if (global.view.terminal_active) {
        if (global.view.terminal.modified) {
            plat_draw_log(&global.view.terminal, 0, 0, false);
        }
    } else {
        // Board needs to be updated
        if (global.view.refresh) {
            global.state.prev_cursor = -1;
            plat_draw_board();
            if (global.view.info_panel.modified) {
                plat_draw_log(&global.view.info_panel, plat_core_get_status_x(), 0, true);
            }
            // If the menu is not hideen, it also needs to draw, on top
            // of the updated board
            if (!(global.view.mc.df & MENU_DRAW_HIDDEN)) {
                global.view.mc.df = MENU_DRAW;
            }
        }

        // Update cursor and selection if needed
        if (global.state.includes_me && (global.state.prev_cursor != global.state.cursor)) {
            if (global.state.prev_cursor >= 0) {
                plat_draw_square(global.state.prev_cursor);  // erase old cursor
            }
            if (global.state.selector_index) {
                // show selected tile in its cursor
                plat_draw_highlight(global.state.selector[0], HCOLOR_SELECTED);
                if (global.state.cursor != global.state.selector[0]) {
                    // show a second (basic) cursor if it's moved from selected[0]
                    plat_draw_highlight(global.state.cursor, HCOLOR_VALID);
                }
            } else {
                // Show the basic cursor
                plat_draw_highlight(global.state.cursor, HCOLOR_VALID);
            }
            global.state.prev_cursor = global.state.cursor;
        }
    }
}

/*-----------------------------------------------------------------------*/
void app_error(bool fatal, const char *error_text) {
    plat_core_active_term(1);
    log_add_line(&global.view.terminal, error_text, -1);
    log_add_line(&global.view.terminal, "Press a key", -1);
    plat_draw_log(&global.view.terminal, 0, 0, MENU_COLOR_TITLE);
    plat_draw_update();
    plat_core_key_wait_any();
    if (fatal) {
        plat_core_exit();
    }
    plat_net_shutdown();
    app_set_state(APP_STATE_OFFLINE);
}

/*-----------------------------------------------------------------------*/
void app_set_state(uint8_t new_state) {
    switch (new_state) {
        case APP_STATE_OFFLINE:
            // Reset the login user name to Guest if it isn't a registered name (1 == yes option)
            if (ui_pregame_menu_options_menu.menu_items[UI_LOGIN_OPTIONS_REGISTERED].selected != 1) {
                strcpy(global.ui.user_name, "Guest");
            };
            // Bring up the pre-game (offline) UI
            menu_set(&ui_pregame_menu);
            plat_core_active_term(0);
            global.app.tick = app_state_offline;
            break;

        case APP_STATE_ONLINE_INIT:
            // Set some state
            plat_net_send("\nset bell 0\nset seek 0\nset style 12\nset autoflag 1\n");
            // Set up the one-time variables that need init
            global.ui.my_game_type = ui_game_types[0];
            // These also send commands to FICS to configure the variables
            // There's a trigger on the last one, which will cause the game to become "online"
            ui_set_item_target(&ui_settings_menu_items[UI_SETTINGS_START_TIME], UI_VARIABLE_TIME, ui_game_start_lengths[0]);
            ui_set_item_target(&ui_settings_menu_items[UI_SETTINGS_INCREMENTALTIME], UI_VARIABLE_INC, ui_game_increments[0]);
            ui_set_item_target(&ui_settings_menu_items[UI_SETTINGS_MAXRATINGMATCH], UI_VARIABLE_AVAILMAX, ui_settings_menu_items[UI_SETTINGS_MAXRATINGMATCH].edit_target);
            ui_set_item_target(&ui_settings_menu_items[UI_SETTINGS_MINRATINGMATCH], UI_VARIABLE_AVAILMIN, ui_settings_menu_items[UI_SETTINGS_MINRATINGMATCH].edit_target);
            ui_settings_menu_items[UI_SETTINGS_MAXRATINGMATCH].item_state = MENU_STATE_ENABLED;
            ui_settings_menu_items[UI_SETTINGS_MINRATINGMATCH].item_state = MENU_STATE_ENABLED;
            // Keep using the offline loop as that has the ability to cancel back to offline should anything go wrong
            break;

        case APP_STATE_ONLINE:
            menu_set(&ui_in_game_menu);
            plat_core_active_term(0);
            global.app.tick = app_state_online;
            break;
    }
    global.app.state = new_state;
}

/*-----------------------------------------------------------------------*/
void app_state_offline() {
    // Part 1 whne the menu is on-screen
    if (global.app.selection != MENU_SELECT_NONE) {
        if ((global.app.selection == MENU_SELECT_BACK && !global.view.mc.nm) ||
                global.os.input_event.code == INPUT_QUIT ||
                global.app.selection == UI_MENU_MAIN_QUIT) {
            // User backs out of top menu, or user
            // chose the quit menu option, or the
            // user quit app (closed window or some quit hotkey)
            global.app.quit = true;
        } else if (global.app.selection == UI_MENU_MAIN_PLAY) {
            // Hide the menu
            global.view.mc.df |= MENU_DRAW_HIDDEN;
            // Make the terminal active
            plat_core_active_term(1);
            // Start the connection
            fics_init();
        }
    }

    // Part 2 - When a connection to the server is underway (menu is hidden)
    if (global.view.mc.df & MENU_DRAW_HIDDEN) {
        if (global.os.input_event.code == INPUT_BACK) {
            // If the user gives up, Stop the connection attempt
            plat_net_shutdown();
            app_set_state(APP_STATE_OFFLINE);
        }
    }
}

/*-----------------------------------------------------------------------*/
void app_state_online() {
    if (global.app.selection != MENU_SELECT_NONE) {
        // If a menu is active and a selection was made
        switch (global.app.selection) {
            case UI_MENU_INGAME_NEW:        // Play
                // Get the cursor somewhere sensible but make it not draw
                // if something goes wrong
                global.state.includes_me = false;
                global.state.cursor = -1;
                fics_play(global.setup.use_seek);
                break;

            case UI_MENU_INGAME_RESIGN:
                plat_net_send("resign");
                break;

            case UI_MENU_INGAME_UNOBSERVE:
                plat_net_send("unobserve");
                break;

            case UI_MENU_INGAME_STOP_SEEK:
                ui_in_game_menu.menu_items[UI_MENU_INGAME_NEW].item_state = MENU_STATE_ENABLED;
                ui_in_game_menu.menu_items[UI_MENU_INGAME_STOP_SEEK].item_state = MENU_STATE_HIDDEN;
                global.view.mc.df = MENU_DRAW_REDRAW;
                plat_net_send("unseek");
                break;

            case UI_MENU_INGAME_TERMINAL:   // Terminal
                // Make the switch to terminal as though VIEW_TOGGLE was pressed
                global.os.input_event.code = INPUT_VIEW_TOGGLE;
                break;

            case UI_MENU_INGAME_QUIT:       // Quit
                fics_shutdown();
                break;

            default:
                break;
        }
    }

    // If the UI is not visible
    if (global.view.mc.df & MENU_DRAW_HIDDEN) {
        if (global.os.input_event.code == INPUT_BACK) {
            // It's not a case of unhiding it, it has been popped so
            // set it up to run
            menu_set(&ui_in_game_menu);
        } else if (global.state.includes_me) {
            // I am a participant in this app, there's a cursor
            app_user_input();
        }
    }

    // On TAB, switch to terminal, whether menu is up or not
    if (global.os.input_event.code == INPUT_VIEW_TOGGLE) {
        // toggle to the terminal view in observed games as well
        app_terminal();
    }
}

/*-----------------------------------------------------------------------*/
void app_user_input() {
    if (global.os.input_event.code == INPUT_MOUSE_MOVE || global.os.input_event.code == INPUT_MOUSE_CLICK) {
        uint8_t cursor = plat_core_mouse_to_cursor();
        if (cursor < 64) {
            global.state.cursor = cursor;
        }
        if (global.os.input_event.code == INPUT_MOUSE_CLICK) {
            global.os.input_event.code = INPUT_SELECT;
        }
    }

    if (global.os.input_event.code == INPUT_KEY) {
        switch (toupper(global.os.input_event.key_value)) {
            case 'W':
                global.os.input_event.code = INPUT_UP;
                break;

            case 'A':
                global.os.input_event.code = INPUT_LEFT;
                break;

            case 'S':
                global.os.input_event.code = INPUT_DOWN;
                break;

            case 'D':
                global.os.input_event.code = INPUT_RIGHT;
                break;

            default:
                break;
        }
    }

    switch (global.os.input_event.code) {
        case INPUT_UP:              // global.state.cursor
            global.state.cursor -= 8;
            if (global.state.cursor < 0) {
                global.state.cursor += 64;
            }
            break;

        case INPUT_DOWN:            // global.state.cursor
            global.state.cursor += 8;
            if (global.state.cursor > 63) {
                global.state.cursor -= 64;
            }
            break;

        case INPUT_LEFT:            // global.state.cursor
            if ((global.state.cursor & 7) == 0) {
                global.state.cursor += 7;
            } else {
                global.state.cursor--;
            }
            break;

        case INPUT_RIGHT:           // global.state.cursor
            if ((global.state.cursor & 7) == 7) {
                global.state.cursor -= 7;
            } else {
                global.state.cursor++;
            }
            break;

        case INPUT_SELECT:          // Lock in the 2 parts of a move
            // You can select (or deselct) the source piece, even when it is you opponents turn
            if (global.state.my_move || !global.state.selector_index || (global.state.selector_index && global.state.cursor == global.state.selector[0])) {
                uint8_t piece;
                bool side;
                piece = fics_letter_to_piece(global.state.chess_board[global.state.cursor]);
                side = piece & PIECE_WHITE ? SIDE_WHITE : SIDE_BLACK;
                piece &= ~PIECE_WHITE;
                // First selection or clearing first selection
                if (!global.state.selector_index || global.state.cursor == global.state.selector[0]) {
                    // Have to select a piece
                    if (!piece || side != global.state.my_side) {
                        break;
                    }
                } else {
                    // Destination can't be own piece
                    if (piece && side == global.state.my_side) {
                        break;
                    }
                }
                global.state.selector[global.state.selector_index++] = global.state.cursor;
                if (global.state.selector_index == 2) {
                    if (global.state.selector[0] != global.state.selector[1]) {
                        global.state.move_str[0] = 'a' + (global.state.selector[0] & 7);
                        global.state.move_str[1] = '8' - (global.state.selector[0] / 8);
                        global.state.move_str[3] = 'a' + (global.state.selector[1] & 7);
                        global.state.move_str[4] = '8' - (global.state.selector[1] / 8);
                        plat_net_send(global.state.move_str);
                    }
                    global.state.selector_index = 0;
                    plat_draw_square(global.state.selector[0]);
                }
                // Force a redraw of the cursor
                global.state.prev_cursor = -1;
            }
            break;

        case INPUT_SAY:
            input_text(global.view.say_buffer + 4, sizeof(global.view.say_buffer) - 5, FILTER_ALLOW_ALL);
            if(global.view.say_buffer[4]) {
                plat_net_send(global.view.say_buffer);
                // Terminate the string for nextg chat
                global.view.say_buffer[4] = '\0';
            }
            break;

        default:
            break;
    }
}
