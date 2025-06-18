/*
 *  global.h
 *  RetroMate
 *
 *  By S. Wessels, 2025.
 *  This is free and unencumbered software released into the public domain.
 *
 */

#ifndef _GLOBALS_H_
#define _GLOBALS_H_

// Include for types, globally
#include <stddef.h>     // NULL
#include <stdint.h>     // *int*_t
#include <stdbool.h>    // bool

#include "app.h"
#include "fics.h"
#include "log.h"
#include "menu.h"
#include "plat.h"
#include "ui.h"
#include "usrinput.h"

// Helper
#define AS(arr) (sizeof(arr) / sizeof((arr)[0]))
#define MIN(a,b)    ((a)<(b) ? (a) : (b))
#define MAX(a,b)    ((a)>(b) ? (a) : (b))
#define UNUSED(x)   (void)(x)


// For plat_core_key_input
typedef enum {
    INPUT_NONE = 0,
    INPUT_QUIT,
    INPUT_BACK,
    INPUT_SELECT,
    INPUT_LEFT,
    INPUT_RIGHT,
    INPUT_UP,
    INPUT_DOWN,
    INPUT_VIEW_TOGGLE,
    INPUT_VIEW_PAN_LEFT,
    INPUT_VIEW_PAN_RIGHT,
    INPUT_BACKSPACE,
    INPUT_SAY,
    INPUT_KEY,
    INPUT_MOUSE_CLICK,
    INPUT_MOUSE_MOVE,
    INPUT_UNKNOWN
} input_code_t;

typedef struct _input_event {
    input_code_t code;  // Class of input
    char key_value;     // Valid if code == INPUT_KEY
    int mouse_x;        // Valid if code = INPUT_MOUSE_*
    int mouse_y;        // Valid if code = INPUT_MOUSE_*
} input_event_t;

// Chess pieces
enum {
    NONE,
    ROOK,
    KNIGHT,
    BISHOP,
    QUEEN,
    KING,
    PAWN,
};

#define HCOLOR_SELECTED             0
#define HCOLOR_VALID                1

#define SIDE_BLACK                  0
#define SIDE_WHITE                  1
#define PIECE_WHITE                 128

// Configuration string sizes
#define MAX_FICS_UNAME_LEN          18
#define MAX_FICS_SERVER_NAME_LEN    64
#define MAX_FICS_PORT_LEN           5

// Offset in stats log where messages will appear
#define FICS_STATSLOG_MSG_ROW       15

// Function that's called by main to tick over the whole application,
// whatever the state
typedef void (*app_tick_t)(void);

// These are categories of global variables.  This is only for organization -
// a sort-of namespace effect

// Interaction with the OS
typedef struct _os {
    input_event_t input_event;
} os_t;

// How/when the information is presented to the user
typedef struct _view {
    tLog info_panel;
    tLog terminal;
    bool terminal_active;
    uint8_t pan_value;
    menu_cache_t mc;
    bool refresh;
    char cursor_char[2];
    char scratch_buffer[81];
    char say_buffer[51];
} view_t;

// Configuration variables set from the UI
typedef struct _ui {
    char user_name[MAX_FICS_UNAME_LEN];
    char user_password[MAX_FICS_UNAME_LEN];
    char server_name[MAX_FICS_SERVER_NAME_LEN];
    char server_port_str[MAX_FICS_PORT_LEN];
    int  server_port;
    uint16_t my_rating;
    char my_rating_type[2];
    char *my_game_type;
    // char my_game_type[11];
} ui_t;

// How a chess game is configured/matched
typedef struct _setup {
    bool use_seek;
    char seek_cmd[16];
    char starting_time_srt[4 + 1];
    char incremental_time_str[4 + 1];
    char min_rating_str[4 + 1];
    char max_rating_srt[4 + 1];
    int starting_time;
    int incremental_time;
    int min_rating;
    int max_rating;
} setup_t;

// Overall application elements
typedef struct _app {
    app_tick_t tick;
    uint8_t state;
    uint8_t selection;
    bool quit;
} app_t;

// State variables for differenrt phases of the game (application)
typedef struct _state {
    char chess_board[65];
    char move_str[6];
    bool game_active;
    bool includes_me;
    bool my_move;
    bool my_side;
    uint8_t server_state;
    int8_t cursor;
    int8_t prev_cursor;
    uint8_t selector_index;
    uint8_t selector[2];
} state_t;

// Specific to the telnet parsing
typedef struct _fics {
    const char *trigger_text;
    fics_match_callback_t match_callback;
    fics_new_data_callback_t new_data_callback;
} fics_t;

// Where a frame coming from the fics server is unpacked
typedef struct _frame {
    char color_to_move[1 + 1];
    char double_pawn_push[2 + 1];
    char w_can_castle_l[1 + 1];
    char w_can_castle_s[1 + 1];
    char b_can_castle_l[1 + 1];
    char b_can_castle_s[1 + 1];
    char moves_since_irreversible[3 + 1];
    char game_number[6 + 1];
    char w_name[18 + 1];
    char b_name[18 + 1];
    char my_relation_to_game[2 + 1];
    char initial_time[3 + 1];
    char time_increment[3 + 1];
    char w_strength[3 + 1];
    char b_strength[3 + 1];
    char w_remaining_time[3 + 1];
    char b_remaining_time[3 + 1];
    char move_number[3 + 1];
    char previous_move[7 + 1];
} frame_t;

typedef struct _text {
    char *side_label[2];
    char *login_error;
    char *game_number;
    char *word_last;
    char *word_next;
    char *word_spaces;
    char *word_strength;
    char *word_time;
    char *title_line1;
    char *title_line2;
} text_t;


// All of the global variables together
typedef struct _global {
    os_t        os;
    view_t      view;
    ui_t        ui;
    setup_t     setup;
    app_t       app;
    state_t     state;
    fics_t      fics;
    frame_t     frame;
    text_t      text;
} global_t;

// The instance of all the global variables
extern global_t global;

#endif //_GLOBALS_H_
