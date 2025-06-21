/*
 *  fics.c
 *  RetroMate
 *
 *  By S. Wessels, 2025.
 *  This is free and unencumbered software released into the public domain.
 *
 */

#include <stdlib.h> // atoi
#include <string.h>
#include <ctype.h>  // is*

#include "global.h"


// Triggers in the match callback
#define FICS_TRIGGER_LOGIN          "login:"
#define FICS_TRIGGER_LOGGED_IN      "enter the server as \""
#define FICS_TRIGGER_MIN_SET        "set to 0."
#define FICS_TRIGGER_CLOSED_URL     "(http://www.freechess.org)."

// Triggers in the data callback
#define FICS_DATA_CREATING          "Creating"
#define FICS_DATA_GAME_OVER         "{Game "
#define FICS_DATA_STYLE12           "<12>"
#define FICS_DATA_SAYS              "says: "
#define FICS_DATA_REMOVING          "Removing game"
#define FICS_DATA_QUIESCENCE        "nor examining a game."
#define FICS_DATA_PASSWORD          "password:"
#define FICS_DATA_REGISTERED        "Starting FICS"
#define FICS_DATA_BAD_PASSWORD      "Invalid password!"

// Commands that are sent
#define FICS_CMD_PLAY               "play "
#define FICS_CMD_QUIT               "quit"
#define FICS_CMD_REFRESH            "refresh"
#define FICS_CMD_S12REFRESH         "set style 12\nrefresh"
#define FICS_CMD_SOUGHT             "sought"

enum {
    SOUGHT_GAME_NUM,                // 0
    SOUGHT_RANKING,                 // 1
    SOUGHT_USER_NAME,               // 2
    SOUGHT_START_TIME,              // 3
    SOUGHT_INC_TIME,                // 4
    SOUGHT_RATED,                   // 5
    SOUGHT_GAME_TYPE,               // 6
    SOUGHT_START_COLOR,             // 7
    SOUGHT_RANGE,                   // 8
    SOUGHT_EXTRA,                   // 9
    SOUGHT_COUNT                    // 10
};

// Forward declare
static void fics_ndcb_login_flow(const char *buf, int len);
static void fics_ndcb_update_from_server(const char *buf, int len);

/*-----------------------------------------------------------------------*/
static void fics_add_status_log(const char *str1, const char *str2) {
    char *ptr = global.view.scratch_buffer;
    while (*str1) {
        *ptr++ = *str1++;
    }
    while (*str2) {
        *ptr++ = *str2++;
    }
    log_add_line(&global.view.info_panel, global.view.scratch_buffer, ptr - global.view.scratch_buffer);
}

/*-----------------------------------------------------------------------*/
static void fics_add_stats(bool side) {
    if (side) { // white
        fics_add_status_log(global.text.word_spaces, global.text.side_label[SIDE_WHITE]);
        fics_add_status_log("", global.frame.w_name);
        fics_add_status_log(global.text.word_strength, global.frame.w_strength);
        fics_add_status_log(global.text.word_time, global.frame.w_remaining_time);
    } else {    // Black
        fics_add_status_log(global.text.word_spaces, global.text.side_label[SIDE_BLACK]);
        fics_add_status_log("", global.frame.b_name);
        fics_add_status_log(global.text.word_strength, global.frame.b_strength);
        fics_add_status_log(global.text.word_time, global.frame.b_remaining_time);
    }
    log_add_line(&global.view.info_panel, "\n", 1);
}

/*-----------------------------------------------------------------------*/
// Copy out data from src, up to the next ' ' in src, or till max_len
// copied out. Advance src to after the ' ' character (even if max_len was
// copied and no ' ' was found)
static const char *fics_copy_data(char *dest, const char *src, uint8_t max_len) {
    while (max_len) {
        if (*src != ' ') {
            *dest++ = *src++;
            max_len--;
        } else {
            break;
        }
    }
    *dest = '\0';
    while (*src != ' ') {
        src++;
    }
    return ++src;
}

/*-----------------------------------------------------------------------*/
static void fics_format_stats_message(const char *message, int len, char delimiter) {
    const char *start = message;
    const char *line_break = NULL;
    bool end_of_message = false;

    // Put the message in the message section by adjusting the size and head
    global.view.info_panel.size = global.view.info_panel.head = FICS_STATSLOG_MSG_ROW;
    // And also updating the dest ptr
    global.view.info_panel.dest_ptr = global.view.info_panel.buffer + (global.view.info_panel.head * global.view.info_panel.cols);
    plat_draw_clear_statslog_area(FICS_STATSLOG_MSG_ROW);

    while (1) { // or start_message != \n or } maybe?
        while (len && *message != ' ' && *message != delimiter) {
            len--;
            message++;
        }
        if (!len || *message == delimiter) {
            end_of_message = true;
        }
        // end of message or word
        if (message - start >= global.view.info_panel.cols || end_of_message) {
            if (line_break && !(end_of_message && (message - start < global.view.info_panel.cols))) {
                log_add_line(&global.view.info_panel, start, line_break - start);
                start = line_break + 1;
                line_break = NULL;
                continue;
            } else {
                log_add_line(&global.view.info_panel, start, message - start);
                start = message + 1;
            }
        } else {
            line_break = message;
            len--;
            message++;
        }
        if (end_of_message) {
            break;
        }
    }
}

#ifdef __APPLE2__
#pragma code-name(push, "LOWCODE")
#endif

/*-----------------------------------------------------------------------*/
static const char *fics_next_number(const char **source, int *len, int *word_len, bool start_of_line) {
    const char *number_start;
    const char *scan = *source;
    int length = *len;

    while (length && scan) {
        // skip leading whitespace
        while (length && isspace(*scan)) {
            length--;
            scan++;
        }

        // if ran buffer out, no number
        if (!length) {
            break;
        }

        number_start = scan;
        while (length && isdigit(*scan) || *scan == '+' || *scan == '-') {
            scan++;
            length--;
        }

        // was it a number ending in a space
        if (*scan == ' ') {
            *source = scan;
            *len = length;
            *word_len = scan - number_start;
            return number_start;
        } else {
            // it wasn't a number ending in a space so
            if (start_of_line) {
                // skip to the next line
                while (length && /*scan &&*/ *scan != '\n') { // I don't think scan can be 0?
                    length--;
                    scan++;
                }
            } else {
                break;
            }
        }
    }

    *len = 0;
    *source = 0;
    *word_len = 0;
    return NULL;
}

/*-----------------------------------------------------------------------*/
static const char *fics_next_word(const char **source, int *len, int *word_len) {
    const char *word_start;
    const char *scan = *source;
    int length = *len;

    while (length && scan) {
        // skip leading whitespace
        while (length && isspace(*scan)) {
            length--;
            scan++;
        }

        // if ran buffer out, no word
        if (!length) {
            break;
        }

        // Find the end of the word
        word_start = scan;
        while (length && isprint(*scan) && !isspace(*scan)) {
            scan++;
            length--;
        }

        // Update the parameters
        *source = scan;
        *len = length;
        *word_len = scan - word_start;
        return word_start;
    }

    // No word found
    *len = 0;
    *source = 0;
    *word_len = 0;
    return NULL;
}

/*-----------------------------------------------------------------------*/
static const char *fics_strnstr(const char *haystack, int haystack_length, const char *needle) {
    int j, i = 0;
    while (i < haystack_length) {
        if (haystack[i] == *needle) {
            j = 1;
            while (needle[j] && haystack[i + j] == needle[j]) {
                j++;
            }
            if (!needle[j]) {
                return haystack + i;
            }
        }
        i++;
    }
    return 0;
}

/*-----------------------------------------------------------------------*/
static void fics_tcb_closed(const char *buf, int len, const char *match) {
    UNUSED(buf);
    UNUSED(len);
    UNUSED(match);
    app_set_state(APP_STATE_OFFLINE);
}

/*-----------------------------------------------------------------------*/
static void fics_tcb_login(const char *buf, int len, const char *match) {
    UNUSED(buf);
    UNUSED(len);
    UNUSED(match);

    // Remove the trigger callback
    fics_set_trigger_callback(NULL, NULL);
    // Install a login flow callback
    fics_set_new_data_callback(fics_ndcb_login_flow);
    plat_net_send(global.ui.user_name);
}

/*-----------------------------------------------------------------------*/
static void fics_tcb_online(const char *buf, int len, const char *match) {
    // Stop the trigger callbacks
    fics_set_trigger_callback(NULL, NULL);
    // Add a regular data callback
    fics_set_new_data_callback(fics_ndcb_update_from_server);
    app_set_state(APP_STATE_ONLINE);
}

/*-----------------------------------------------------------------------*/
static void fics_ndcb_login_flow(const char *buf, int len) {
    bool login_error = false;
    const char *error_string;
    const char *parse_point = buf;
    while (len > 0) {
        char character = *parse_point;
        if (character == FICS_DATA_PASSWORD[0] && 0 == strncmp(parse_point, FICS_DATA_PASSWORD, (sizeof(FICS_DATA_PASSWORD) - 1))) {
            if(ui_pregame_menu_options_menu.menu_items[UI_LOGIN_OPTIONS_REGISTERED].selected == 1) {
                // Prompting for a password on a registered account
                plat_net_send(global.ui.user_password);
            } else {
                login_error = 1;
                error_string = "Registered account";
                break;
            }
        } else if (character == FICS_DATA_REGISTERED[0] && 0 == strncmp(parse_point, FICS_DATA_REGISTERED, (sizeof(FICS_DATA_REGISTERED) - 1))) {
            // Login as registered user a success, go to online init
            fics_set_trigger_callback(FICS_TRIGGER_MIN_SET, fics_tcb_online);
            app_set_state(APP_STATE_ONLINE_INIT);
        } else if (character == FICS_DATA_BAD_PASSWORD[0] && 0 == strncmp(parse_point, FICS_DATA_BAD_PASSWORD, (sizeof(FICS_DATA_BAD_PASSWORD) - 1))) {
            // Password wasn't accepted
            login_error = true;
            error_string = "Invalid password";
            break;
        } else if (character == FICS_TRIGGER_LOGGED_IN[0] && 0 == strncmp(parse_point, FICS_TRIGGER_LOGGED_IN, (sizeof(FICS_TRIGGER_LOGGED_IN) - 1))) {
            if(ui_pregame_menu_options_menu.menu_items[UI_LOGIN_OPTIONS_REGISTERED].selected == 1) {
                // User name isn't registered, but was available, but that's not what the user wanted, so go offline
                login_error = true;
                error_string = "Not a registered account";
                break;
            } else {
                fics_set_trigger_callback(FICS_TRIGGER_MIN_SET, fics_tcb_online);
                app_set_state(APP_STATE_ONLINE_INIT);
            }
        }
        len--;
        parse_point++;
    }

    if(login_error) {
        app_error(false, error_string);
        fics_set_trigger_callback(NULL, NULL);
        plat_net_shutdown();
        app_set_state(APP_STATE_OFFLINE);
    }
}

/*-----------------------------------------------------------------------*/
static void fics_ndcb_sought_list(const char *buf, int len) {
    uint16_t delta;
    uint16_t rating;
    uint8_t i;
    bool error;
    const char *sought_word[SOUGHT_COUNT];
    int sought_word_len[SOUGHT_COUNT];
    char game_number_str[6];
    bool found_a_game = false;
    const char *parse_point = buf;
    int prev_lines_processed = 0;
    int lines_processed = 0;
    uint16_t rating_delta = 65535;

    while (len) {
        if (prev_lines_processed != lines_processed) {
            prev_lines_processed = lines_processed;
            // Don't pick a game with a color preference
            if (sought_word_len[SOUGHT_START_COLOR]) {
                continue;
            }
            // Make sure there are no strings attached
            if (sought_word_len[SOUGHT_EXTRA]) {
                continue;
            }
            // Make sure it's the type of game I want
            if (global.ui.my_game_type && 0 != strncmp(global.ui.my_game_type, sought_word[SOUGHT_GAME_TYPE], sought_word_len[SOUGHT_GAME_TYPE])) {
                continue;
            }
            // Make sure it's rated or unrated as I desire
            if (global.ui.my_rating_type[0] != sought_word[SOUGHT_RATED][0]) {
                continue;
            }
            // See if this offer is closer to my rating
            rating = atoi(sought_word[SOUGHT_RANKING]);
            delta = abs(rating - global.ui.my_rating);
            if (delta < rating_delta && sought_word_len[SOUGHT_GAME_NUM] < sizeof(game_number_str) - 1) {
                rating_delta = delta;
                strncpy(game_number_str, sought_word[SOUGHT_GAME_NUM], sought_word_len[SOUGHT_GAME_NUM]);
                game_number_str[sought_word_len[SOUGHT_GAME_NUM]] = '\0';
                found_a_game = true;
            }
        }
        sought_word[SOUGHT_GAME_NUM] = fics_next_number(&parse_point, &len, &sought_word_len[SOUGHT_GAME_NUM], true);     // Game #
        sought_word[SOUGHT_RANKING] = fics_next_number(&parse_point, &len, &sought_word_len[SOUGHT_RANKING], false);       // Rating
        // Not a number here could denote the end
        if (!sought_word[SOUGHT_RANKING]) {
            // Is it the end of the list
            if (sought_word_len[SOUGHT_GAME_NUM] && atoi(sought_word[SOUGHT_GAME_NUM]) == lines_processed) {
                // Ads was not a number - end of all soughts, process now
                fics_set_new_data_callback(fics_ndcb_update_from_server);
                if (found_a_game) {
                    strcpy(global.view.scratch_buffer, FICS_CMD_PLAY);
                    strcat(global.view.scratch_buffer, game_number_str);
                    // fics_concat_to_scratch(FICS_CMD_PLAY, game_number_str);
                    plat_net_send(global.view.scratch_buffer);
                    // Asking for the game - may not start though so re-enable the menu
                    ui_in_game_menu.menu_items[UI_MENU_INGAME_NEW].item_state = MENU_STATE_ENABLED;
                    return;
                }
                // No game found, exit and try a seek
                break;
            } else {
                // Garbage in - just keep looking for a number at the start of a line
                continue;
            }
        }
        sought_word[SOUGHT_USER_NAME] = fics_next_word(&parse_point, &len, &sought_word_len[SOUGHT_USER_NAME]);        // User Name
        sought_word[SOUGHT_START_TIME] = fics_next_number(&parse_point, &len, &sought_word_len[SOUGHT_START_TIME], false); // Start Time
        sought_word[SOUGHT_INC_TIME] = fics_next_number(&parse_point, &len, &sought_word_len[SOUGHT_INC_TIME], false);     // Inc Time
        sought_word[SOUGHT_RATED] = fics_next_word(&parse_point, &len, &sought_word_len[SOUGHT_RATED]);                // Rated/Unrated
        sought_word[SOUGHT_GAME_TYPE] = fics_next_word(&parse_point, &len, &sought_word_len[SOUGHT_GAME_TYPE]);        // Game Type (blitz/suicide)
        sought_word[SOUGHT_START_COLOR] = fics_next_word(&parse_point, &len, &sought_word_len[SOUGHT_START_COLOR]);    // Could be [white] or [black]

        // Check if all fields read something
        error = false;
        for (i = 0; i < 8; i++) {
            if (!sought_word_len[i]) {
                error = true;
                break;
            }
        }
        if (error) {
            // If not all fields have a value, there's an error
            continue;
        }
        // This is a valid line
        lines_processed++;
        if (*sought_word[SOUGHT_START_COLOR] == '[') {
            sought_word[SOUGHT_RANGE] = fics_next_number(&parse_point, &len, &sought_word_len[SOUGHT_RANGE], 0);       // Rating
        } else {
            // This is correct, but uneccesary as I don't care
            // sought_word_len[SOUGHT_RANGE] = sought_word_len[SOUGHT_START_COLOR];
            // sought_word[SOUGHT_RANGE] = sought_word[SOUGHT_START_COLOR];
            sought_word_len[SOUGHT_START_COLOR] = 0;
        }
        while (len && *parse_point == ' ') {
            len--;
            parse_point++;
        }
        if (len && isspace(*parse_point)) {
            sought_word_len[SOUGHT_EXTRA] = 0;
            continue;
        }
        sought_word[SOUGHT_EXTRA] = fics_next_word(&parse_point, &len, &sought_word_len[SOUGHT_EXTRA]);                // (f)ormula, (m)anual, ...
    }
    fics_set_new_data_callback(fics_ndcb_update_from_server);
    fics_play(true);
}

/*-----------------------------------------------------------------------*/
static void fics_ndcb_update_from_server(const char *buf, int len) {
    const char *parse_point = buf;
    const char *parse_start;
    while (len > 0) {
        char character = *parse_point;
        if (character == FICS_DATA_STYLE12[0] && 0 == strncmp(parse_point, FICS_DATA_STYLE12, (sizeof(FICS_DATA_STYLE12) - 1))) {
            // In a style 12 game
            uint8_t i;
            char *cb = global.state.chess_board;
            parse_start = parse_point;
            global.state.game_active = true;
            parse_point += 5;
            if (*parse_point == 'I') {
                // Illegal move - let's get the state back
                // I could keep a pre-move state and reinstate that, or just do this.
                // A bit heavey-handed, but simple
                plat_net_send(FICS_CMD_REFRESH);
                return;
            }
            global.view.refresh = true;
            for (i = 0; i < 8; i++) {
                strncpy(cb, parse_point, 8);
                cb += 8;
                parse_point += 9;
            }
            parse_point = fics_copy_data(global.frame.color_to_move, parse_point, 1);
            parse_point = fics_copy_data(global.frame.double_pawn_push, parse_point, 2);
            parse_point = fics_copy_data(global.frame.w_can_castle_l, parse_point, 1);
            parse_point = fics_copy_data(global.frame.w_can_castle_s, parse_point, 1);
            parse_point = fics_copy_data(global.frame.b_can_castle_l, parse_point, 1);
            parse_point = fics_copy_data(global.frame.b_can_castle_s, parse_point, 1);
            parse_point = fics_copy_data(global.frame.moves_since_irreversible, parse_point, 3);
            parse_point = fics_copy_data(global.frame.game_number, parse_point, 6);
            parse_point = fics_copy_data(global.frame.w_name, parse_point, 18);
            parse_point = fics_copy_data(global.frame.b_name, parse_point, 18);
            parse_point = fics_copy_data(global.frame.my_relation_to_game, parse_point, 2);
            parse_point = fics_copy_data(global.frame.initial_time, parse_point, 3);
            parse_point = fics_copy_data(global.frame.time_increment, parse_point, 3);
            parse_point = fics_copy_data(global.frame.w_strength, parse_point, 3);
            parse_point = fics_copy_data(global.frame.b_strength, parse_point, 3);
            parse_point = fics_copy_data(global.frame.w_remaining_time, parse_point, 3);
            parse_point = fics_copy_data(global.frame.b_remaining_time, parse_point, 3);
            parse_point = fics_copy_data(global.frame.move_number, parse_point, 3);
            parse_point = fics_copy_data(global.frame.previous_move, parse_point, 7);

            global.state.includes_me = global.frame.my_relation_to_game[0] == '1' || global.frame.my_relation_to_game[1] == '1';
            global.state.my_move = global.frame.my_relation_to_game[0] == '1';

            if (!global.state.includes_me) {
                global.state.my_side = SIDE_WHITE;
            } else {
                // Derive my color based on whether it's my move or not
                global.state.my_side = global.state.my_move ? *global.frame.color_to_move == 'W' : *global.frame.color_to_move != 'W';
                if (global.state.cursor < 0) {
                    global.state.cursor = global.state.my_side ? 51 : 12;
                }
            }
            if (ui_in_game_menu.menu_items[UI_MENU_INGAME_NEW].item_state == MENU_STATE_ENABLED ||
                ui_in_game_menu.menu_items[UI_MENU_INGAME_STOP_SEEK].item_state == MENU_STATE_ENABLED) {
                if (global.view.info_panel.size > FICS_STATSLOG_MSG_ROW) {
                    plat_draw_clear_statslog_area(FICS_STATSLOG_MSG_ROW);
                }
                ui_in_game_menu.menu_items[UI_MENU_INGAME_NEW].item_state = MENU_STATE_HIDDEN;
                ui_in_game_menu.menu_items[UI_MENU_INGAME_STOP_SEEK].item_state = MENU_STATE_HIDDEN;
                if (global.state.includes_me) {
                    ui_in_game_menu.menu_items[UI_MENU_INGAME_RESIGN].item_state = MENU_STATE_ENABLED;
                } else {
                    ui_in_game_menu.menu_items[UI_MENU_INGAME_UNOBSERVE].item_state = MENU_STATE_ENABLED;
                }
            }
            log_clear(&global.view.info_panel);
            fics_add_status_log(global.text.game_number, global.frame.game_number);
            log_add_line(&global.view.info_panel, "\n", 1);
            fics_add_stats(global.state.my_side);
            fics_add_stats(global.state.my_side ^ 1);
            fics_add_status_log(global.text.word_next, *global.frame.color_to_move == 'W' ? global.text.side_label[SIDE_WHITE] : global.text.side_label[SIDE_BLACK]);
            fics_add_status_log(global.text.word_last, global.frame.previous_move);

            // Move past all this to see if there are more statements to parse (Game Over comes with last
            // move in all cases I observed)
            len -= (parse_point - parse_start);
            while (len && *parse_point != '\n') {
                parse_point++;
                len--;
            }
        } else if (character == FICS_DATA_GAME_OVER[0] && 0 == strncmp(parse_point, FICS_DATA_GAME_OVER, (sizeof(FICS_DATA_GAME_OVER) - 1))) {
            // Game status message received
            global.view.refresh = true;
            // Skip user names
            while (len && *parse_point != ')') {
                parse_point++;
                len--;
            }
            // Skip ) & space
            parse_point += 2;
            len -= 2;
            if (len) {
                // If it's a Creating message, it's still game-on
                if (!(*parse_point == FICS_DATA_CREATING[0] && 0 == strncmp(parse_point, FICS_DATA_CREATING, (sizeof(FICS_DATA_CREATING) - 1)))) {
                    // but if not, it's a game over message
                    global.state.game_active = false;
                }
                // Whatever message, show it
                fics_format_stats_message(parse_point, len, '}');
            }
            // Force a refresh to see what menu item states should be active
            plat_net_send(FICS_CMD_REFRESH);
        } else if (character == FICS_DATA_REMOVING[0] && 0 == strncmp(parse_point, FICS_DATA_REMOVING, (sizeof(FICS_DATA_REMOVING) - 1))) {
            // Force a refresh to see what menu item states should be active
            plat_net_send(FICS_CMD_REFRESH);
        } else if (character == FICS_DATA_QUIESCENCE[0] && 0 == strncmp(parse_point, FICS_DATA_QUIESCENCE, (sizeof(FICS_DATA_QUIESCENCE) - 1))) {
            ui_in_game_menu.menu_items[UI_MENU_INGAME_NEW].item_state = MENU_STATE_ENABLED;
            ui_in_game_menu.menu_items[UI_MENU_INGAME_RESIGN].item_state = MENU_STATE_HIDDEN;
            ui_in_game_menu.menu_items[UI_MENU_INGAME_UNOBSERVE].item_state = MENU_STATE_HIDDEN;
            global.state.game_active = false;
            if (global.view.mc.m && !(global.view.mc.df & MENU_DRAW_HIDDEN)) {
                global.view.mc.df = MENU_DRAW_REDRAW;
            }
        } else if (character == FICS_DATA_SAYS[0] && 0 == strncmp(parse_point, FICS_DATA_SAYS, (sizeof(FICS_DATA_SAYS) - 1))) {
            // says: received - show what was said
            parse_point += (sizeof(FICS_DATA_SAYS) - 1);
            len -= (sizeof(FICS_DATA_SAYS) - 1);
            parse_start = parse_point;
            while (len && *parse_point != '\n') {
                len--;
                parse_point++;
            }
            if (*parse_point == '\n') {
                fics_format_stats_message(parse_start, parse_point - parse_start, '\n');
                global.view.refresh = true;
            }
        }
        len--;
        parse_point++;
    }
}

/*-----------------------------------------------------------------------*/
void fics_init() {
    plat_net_connect(global.ui.server_name, global.ui.server_port);
    if(ui_pregame_menu_options_menu.menu_items[UI_LOGIN_OPTIONS_REGISTERED].selected == 1 &&
        !global.ui.user_password[0]) {
            app_error(false, "Empty Password");
            app_set_state(APP_STATE_OFFLINE);
    } else {
        fics_set_trigger_callback(FICS_TRIGGER_LOGIN, fics_tcb_login);
    }
}

/*-----------------------------------------------------------------------*/
uint8_t fics_letter_to_piece(char letter) {
    static uint8_t text2piece[19] = {
        NONE, NONE, BISHOP, NONE,
        NONE, NONE, NONE, NONE,
        NONE, NONE, NONE, KING,
        NONE, NONE, KNIGHT, NONE,
        PAWN, QUEEN, ROOK
    };
    uint8_t piece = letter == 0x2d ? 0 : letter >= 0x61 ? (letter - 0x60) : (letter - 0x40) | PIECE_WHITE;
    piece = text2piece[piece & 0x1F] | (piece & PIECE_WHITE);
    return piece;
}

/*-----------------------------------------------------------------------*/
void fics_play(bool use_seek) {
    ui_in_game_menu.menu_items[UI_MENU_INGAME_NEW].item_state = MENU_STATE_HIDDEN;
    if(use_seek) {
        ui_in_game_menu.menu_items[UI_MENU_INGAME_STOP_SEEK].item_state = MENU_STATE_ENABLED;
        if(!(global.view.mc.df & MENU_DRAW_HIDDEN)) {
            global.view.mc.df = MENU_DRAW_REDRAW;
        }
        if(global.ui.my_game_type[1] == 'u' || global.ui.my_game_type[1] == 'i' || global.ui.my_game_type[1] == 'r') {
            strcpy(&global.setup.seek_cmd[5], global.ui.my_game_type);
            plat_net_send(global.setup.seek_cmd);
        }
    } else {
        fics_set_new_data_callback(fics_ndcb_sought_list);
        plat_net_send(FICS_CMD_SOUGHT);
    }
}

/*-----------------------------------------------------------------------*/
void fics_set_new_data_callback(fics_new_data_callback_t callback) {
    global.fics.new_data_callback = callback;
}

/*-----------------------------------------------------------------------*/
void fics_set_trigger_callback(const char *text, fics_match_callback_t callback) {
    global.fics.trigger_text = text;
    global.fics.match_callback = callback;
}

/*-----------------------------------------------------------------------*/
void fics_shutdown() {
    fics_set_trigger_callback(FICS_TRIGGER_CLOSED_URL, fics_tcb_closed);
    plat_net_send(FICS_CMD_QUIT);
}

/*-----------------------------------------------------------------------*/
void fics_tcp_recv(const unsigned char *buf, int len) {
    if (len == -1) {
        app_error(false, "TCP recv error.");
        app_set_state(APP_STATE_OFFLINE);
    } else {
        const char *match;
        log_add_line(&global.view.terminal, (const char *)buf, len);
        if (global.fics.match_callback) {
            if ((match = fics_strnstr((const char *)buf, len, global.fics.trigger_text))) {
                (*global.fics.match_callback)((const char *)buf, len, match);
            }
        } else if (global.fics.new_data_callback) {
            (*global.fics.new_data_callback)((const char *)buf, len);
        }
    }
}

#ifdef __APPLE2__
#pragma code-name(pop)
#endif