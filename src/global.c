/*
 *  global.c
 *  RetroMate
 *
 *  By S. Wessels, 2025.
 *  This is free and unencumbered software released into the public domain.
 *
 */

#include "global.h"

/*-----------------------------------------------------------------------*/
global_t global = {
    {
        // os
        {
            // input_event
            INPUT_NONE,                           // input_code_t code
            0,                                    // key_value
            0,                                    // mouse_x
            0                                     // mouse_y
        },
    },
    {
        // view
        {},                                       // info_panel
        {},                                       // terminal
        false,                                    // terminal_active
        0,                                        // pan_value
        {
            // mc (menu cache)
            0,                                    // active menu
            0,                                    // next active menu (0 unless poping a menu)
            0,                                    // draw_flags
            0,                                    // menu's x
            0,                                    // menu's y
            0,                                    // menu's w
            0,                                    // menu's h
            0,                                    // x of Title
            0,                                    // x of menu items
            0,                                    // x of action items
            0,                                    // selected item's y
            0,                                    // max length of all items
            0,                                    // max length of all actions
            0,                                    // working y
        },
        true,                                     // refresh
        {0, ' '},                                 // cursor_char[2]
        "",                                       // scratch_buffer
        "say "                                    // say_buffer
    },
    {
        // ui
        "Guest",                                  // user_name
        "",                                       // user_password
        "freechess.org",                          // server_name
        "5000",                                   // server_port_str
        5000,                                     // server_port
        0,                                        // my_rating
        "\x75",                                   // my_rating_type (in ascii 'u')
        NULL,                                     // my_game_type
    },
    {
        // setup
        false,                                    // use_seek
        "seek ",                                  // seek_cmd
        "15",                                     // starting_time_srt (Standard = default; see ui.c)
        "0",                                      // incremental_time_str
        "0",                                      // min_rating_str
        "9999",                                   // max_rating_srt
        15,                                       // starting_time    (see starting_time_srt)
        0,                                        // incremental_time
        0,                                        // min_rating
        9999,                                     // max_rating
    },
    {
        // app
        NULL,                                     // tick
        0,                                        // state
        0,                                        // selection
        false,                                    // quit
    },
    {
        // state
        {
            // chess_board
        "\x72\x6e\x62\x71\x6b\x62\x6e\x72"        // "rnbqkbnr"
        "\x70\x70\x70\x70\x70\x70\x70\x70"        // "pppppppp"
        "\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d"        // "--------"
        "\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d"        // "--------"
        "\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d"        // "--------"
        "\x2d\x2d\x2d\x2d\x2d\x2d\x2d\x2d"        // "--------"
        "\x50\x50\x50\x50\x50\x50\x50\x50"        // "PPPPPPPP"
        "\x52\x4e\x42\x51\x4b\x42\x4e\x52"        // "RNBQKBNR"
        },
        "a1-a1",                                  // move_str[6]
        false,                                    // game_active
        false,                                    // includes_me
        false,                                    // my_move
        false,                                    // my_side
        0,                                        // server_state
        0,                                        // cursor
        -1,                                       // prev_cursor
        0,                                        // selector_index
        {0, 0},                                   // selector[2]
    },
    {
        // fics
        NULL,                                     // trigger_text
        NULL,                                     // match_callback
        NULL,                                     // new_data_callback
    },
    {
        // frame
        "",                                       //color_to_move
        "",                                       //double_pawn_push
        "",                                       //w_can_castle_l
        "",                                       //w_can_castle_s
        "",                                       //b_can_castle_l
        "",                                       //b_can_castle_s
        "",                                       //moves_since_irreversible
        "",                                       //game_number
        "",                                       //w_name
        "",                                       //b_name
        "",                                       //my_relation_to_game
        "",                                       //initial_time
        "",                                       //time_increment
        "",                                       //w_strength
        "",                                       //b_strength
        "",                                       //w_remaining_time
        "",                                       //b_remaining_time
        "",                                       //move_number
        "",                                       //previous_move
    },
    {
        // text
        {"Black", "White"},                       // global.text.side_label[2];
        "Login Error.",                           // global.text.login_error
        "Game #",                                 // global.text.game_number
        "Last: ",                                 // global.text.word_last
        "Next: ",                                 // global.text.word_next
        "  ",                                     // global.text.word_spaces
        "Strength: ",                             // global.text.word_strength
        "Time: ",                                 // global.text.word_time
        "RetroMate Free Internet Chess Client",   // global.text.title_line1
        "V1.0 by S. Wessels and O. Schmidt 2025", // global.text.title_line2
    }
};
