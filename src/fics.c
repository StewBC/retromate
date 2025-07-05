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

// This files has strings and characters encoded as hex, not as ' ' or "".  That is
// because what comes from the server is ASCII but the target platforms aren't all ASCII.
// The compiler will encode strings as "platform strings" and that won't match the server
// side data.  This is a way to circumvent the compiler encoding these as non-ASCII data.


// Triggers in the match callback
                                    // "login:"
#define FICS_TRIGGER_LOGIN          "\x6C\x6F\x67\x69\x6e\x3a"
                                    // "enter the server as \""
#define FICS_TRIGGER_LOGGED_IN      "\x65\x6e\x74\x65\x72\x20\x74\x68\x65\x20\x73\x65\x72\x76\x65\x72\x20\x61\x73\x20\x22"
                                    // "set to 0."
#define FICS_TRIGGER_MIN_SET        "\x73\x65\x74\x20\x74\x6f\x20\x30\x2e"
                                    // "(http://www.freechess.org)."
#define FICS_TRIGGER_CLOSED_URL     "\x28\x68\x74\x74\x70\x3a\x2f\x2f\x77\x77\x77\x2e\x66\x72\x65\x65\x63\x68\x65\x73\x73\x2e\x6f\x72\x67\x29\x2e"

// Triggers in the data callback
                                    // "Creating"
#define FICS_DATA_CREATING          "\x43\x72\x65\x61\x74\x69\x6e\x67"
                                    // "{Game "
#define FICS_DATA_GAME_OVER         "\x7b\x47\x61\x6d\x65\x20"
                                    // "<12>"
#define FICS_DATA_STYLE12           "\x3c\x31\x32\x3e"
                                    // "says: "
#define FICS_DATA_SAYS              "\x73\x61\x79\x73\x3a\x20"
                                    // "Removing game"
#define FICS_DATA_REMOVING          "\x52\x65\x6d\x6f\x76\x69\x6e\x67\x20\x67\x61\x6d\x65"
                                    // "nor examining a game."
#define FICS_DATA_QUIESCENCE        "\x6e\x6f\x72\x20\x65\x78\x61\x6d\x69\x6e\x69\x6e\x67\x20\x61\x20\x67\x61\x6d\x65\x2e"
                                    // "password:"
#define FICS_DATA_PASSWORD          "\x70\x61\x73\x73\x77\x6f\x72\x64\x3a"
                                    // "Starting FICS"
#define FICS_DATA_REGISTERED        "\x53\x74\x61\x72\x74\x69\x6e\x67\x20\x46\x49\x43\x53"
                                    // "Invalid password!"
#define FICS_DATA_BAD_PASSWORD      "\x49\x6e\x76\x61\x6c\x69\x64\x20\x70\x61\x73\x73\x77\x6f\x72\x64\x21"

// Commands that are sent (are in platform format) and get converted to ASCII
// before being sent (by plat_net_send)
#define FICS_CMD_PLAY               "play "
#define FICS_CMD_QUIT               "quit"
#define FICS_CMD_REFRESH            "refresh"
#define FICS_CMD_S12REFRESH         "set style 12\nrefresh"
#define FICS_CMD_SOUGHT             "sought"

enum {
    FS_STATUS_OKAY,
    FS_STATUS_NEXTLINE,
    FS_STATUS_DONE,
    FS_STATUS_NO_MATCH,
    FS_STATUS_ERROR,
};

fics_data_t fics_data = {
    SOUGHT_GAME_NUM,
    FS_STATUS_OKAY,
    0,
    0,
    {},
    {},
    ""
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

#ifdef __ATARIXL__
#pragma code-name(push, "SHADOW_RAM")
#endif

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


#ifdef __ATARIXL__
#pragma code-name(pop)
#endif

/*-----------------------------------------------------------------------*/
// Copy out data from src, up to the next ' ' in src, or till max_len
// copied out. Advance src to after the ' ' character (even if max_len was
// copied and no ' ' was found)
static const char *fics_copy_data(char *dest, const char *src, uint8_t max_len) {
    while (max_len) {
        if (*src != '\x20') {   // ' '
            *dest++ = *src++;
            max_len--;
        } else {
            break;
        }
    }
    *dest = '\0';
    while (*src != '\x20') {
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
        while (len && *message != '\x20' && *message != delimiter) { // ' '
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

/*-----------------------------------------------------------------------*/
bool fics_isnumeric(char c) {
    if(c >= '\x30' && c <= '\x39' || c == '\x2b' || c == '\x2d') { // '+' '-'
        return true;
    }
    return false;
}

/*-----------------------------------------------------------------------*/
bool fics_isspace(char c) {
    if(c == '\x20' || c == '\x0d' || c == '\x0a' || c == '\x09') {
        return true;
    }
    return false;
}

#ifdef __APPLE2__
#pragma code-name(push, "LOWCODE")
#endif

/*-----------------------------------------------------------------------*/
static void fics_next_number() {
    // Skip whitespace
    while(fics_data.length && fics_isspace(*fics_data.parse_point)) {
        fics_data.length--;
        fics_data.parse_point++;
    }

    // If there's nothing more, done
    if(!fics_data.length) {
        return;
    }

    // If not a number, mark as such
    if(!fics_isnumeric(*fics_data.parse_point)) {
        fics_data.sought_word[fics_data.parse_state] = '\0';
        fics_data.sought_word_len[fics_data.parse_state] = 0;
        fics_data.parse_state++;
    }

    // Extract the number
    fics_data.sought_word[fics_data.parse_state] = fics_data.parse_point;
    while(fics_data.length && fics_isnumeric(*fics_data.parse_point)) {
            fics_data.length--;
            fics_data.parse_point++;
    }

    if(fics_data.length) {
        // Set up the length
        fics_data.sought_word_len[fics_data.parse_state] = fics_data.parse_point - fics_data.sought_word[fics_data.parse_state];
        fics_data.parse_state++;
    } else {
        // This is a serious issue - this record is split - I don't really deal with that
        // So skip till a newline and this record will just be ignored
        fics_data.status = FS_STATUS_NEXTLINE;
    }
}

#ifdef __ATARIXL__
#pragma code-name(push, "SHADOW_RAM")
#endif

/*-----------------------------------------------------------------------*/
static void fics_next_word() {
    // Skip whitespace
    while(fics_data.length && fics_isspace(*fics_data.parse_point)) {
        fics_data.length--;
        fics_data.parse_point++;
    }

    // If there's nothing more, done
    if(!fics_data.length) {
        return;
    }

    // If not a word-start, mark as such
    if(!*fics_data.parse_point) {
        fics_data.sought_word[fics_data.parse_state] = '\0';
        fics_data.sought_word_len[fics_data.parse_state] = 0;
        fics_data.parse_state++;
    }

    // Extract the word
    fics_data.sought_word[fics_data.parse_state] = fics_data.parse_point;
    while(fics_data.length && *fics_data.parse_point && !fics_isspace(*fics_data.parse_point)) {
            fics_data.length--;
            fics_data.parse_point++;
    }

    if(fics_data.length) {
        // set the length
        fics_data.sought_word_len[fics_data.parse_state] = fics_data.parse_point - fics_data.sought_word[fics_data.parse_state];
        fics_data.parse_state++;
    } else {
        // This is a serious issue - this record is split - I don't really deal with that
        // So skip till a newline and this record will just be ignored
        fics_data.status = FS_STATUS_NEXTLINE;
    }
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
    UNUSED(buf);
    UNUSED(len);
    UNUSED(match);

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
            if (ui_pregame_menu_options_menu.menu_items[UI_LOGIN_OPTIONS_REGISTERED].selected == 1) {
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
            if (ui_pregame_menu_options_menu.menu_items[UI_LOGIN_OPTIONS_REGISTERED].selected == 1) {
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

    if (login_error) {
        app_error(false, error_string);
        fics_set_trigger_callback(NULL, NULL);
        plat_net_shutdown();
        app_set_state(APP_STATE_OFFLINE);
    }
}

/*-----------------------------------------------------------------------*/
static void fics_ndcb_sought_list(const char *buf, int len) {
    if(fics_data.status == FS_STATUS_NEXTLINE) {
        while(len && '\x0a' != *buf) {  // '\n'
            len--;
            buf++;
        }
        fics_data.parse_state = SOUGHT_GAME_NUM;
    }
    fics_data.status = FS_STATUS_OKAY;
    fics_data.parse_point = buf;
    fics_data.length = len;

    while(fics_data.status == FS_STATUS_OKAY && fics_data.length) {
        switch(fics_data.parse_state) {
            case SOUGHT_GAME_NUM:
                fics_next_number();
                break;

            case SOUGHT_RANKING:
                fics_next_number();
                if(!fics_data.sought_word_len[SOUGHT_RANKING]) {
                    if(fics_data.game_number_str[0]) {
                        fics_data.status = FS_STATUS_DONE;
                    } else {
                        fics_data.status = FS_STATUS_NO_MATCH;
                    }
                }
            break;

            case SOUGHT_USER_NAME:
                fics_next_word();
            break;

            case SOUGHT_START_TIME:
                fics_next_number();
            break;

            case SOUGHT_INC_TIME:
                fics_next_number();
            break;

            case SOUGHT_RATED:
                fics_next_word();
            break;

            case SOUGHT_GAME_TYPE:
                fics_next_word();
            break;

            case SOUGHT_START_COLOR:
                fics_next_word();
                if(fics_data.sought_word_len[SOUGHT_START_COLOR] > 0) {
                    if(*fics_data.sought_word[SOUGHT_START_COLOR] != '\x5b') { // '['
                        // If the start color isn't a color but is actually the range
                        fics_data.sought_word[SOUGHT_RANGE] = fics_data.sought_word[SOUGHT_START_COLOR];
                        fics_data.sought_word_len[SOUGHT_RANGE] = fics_data.sought_word_len[SOUGHT_START_COLOR];
                        fics_data.sought_word_len[SOUGHT_START_COLOR] = 0;
                        // Go to post range (extra)
                        fics_data.parse_state++;
                    }
                }
            break;

            case SOUGHT_RANGE:
                fics_next_number();
            break;

            case SOUGHT_EXTRA:
                // Start by saying there's no extra
                fics_data.sought_word_len[SOUGHT_EXTRA] = 0;
                // see if the end of the line is here
                while(fics_data.length) {
                    while(*fics_data.parse_point != '\x0a' && fics_isspace(*fics_data.parse_point)) {
                        fics_data.length--;
                        fics_data.parse_point++;
                    }
                    // If not EOL, there's some extra info to skip
                    if(*fics_data.parse_point != '\x0a') {
                        fics_next_word();
                        // keep at extra
                        fics_data.parse_state = SOUGHT_EXTRA;
                    } else {
                        uint16_t rating, delta;

                        // Reset for the next line
                        fics_data.parse_state = SOUGHT_GAME_NUM;

                        // See if this is a fitting game

                        // Don't pick a game with a color preference
                        if (fics_data.sought_word_len[SOUGHT_START_COLOR]) {
                            break;
                        }
                        // Make sure there are no strings attached
                        if (fics_data.sought_word_len[SOUGHT_EXTRA]) {
                            break;
                        }

                        // Make sure it's the type of game I want
                        if (global.ui.my_game_type && 0 != strncmp(global.ui.my_game_type, fics_data.sought_word[SOUGHT_GAME_TYPE], fics_data.sought_word_len[SOUGHT_GAME_TYPE])) {
                            break;
                        }

                        // Make sure it's rated or unrated as I desire
                        if (global.ui.my_rating_type[0] != fics_data.sought_word[SOUGHT_RATED][0]) {
                            break;
                        }

                        // See if this offer is closer to my rating
                        rating = atoi(fics_data.sought_word[SOUGHT_RANKING]);
                        delta = abs(rating - global.ui.my_rating);
                        if (delta < fics_data.rating_delta && fics_data.sought_word_len[SOUGHT_GAME_NUM] < sizeof(fics_data.game_number_str) - 1) {
                            fics_data.rating_delta = delta;
                            strncpy(fics_data.game_number_str, fics_data.sought_word[SOUGHT_GAME_NUM], fics_data.sought_word_len[SOUGHT_GAME_NUM]);
                            fics_data.game_number_str[fics_data.sought_word_len[SOUGHT_GAME_NUM]] = '\0';
                        }
                        break;
                    }
                }
            break;
        }
    }

    switch(fics_data.status) {
        case FS_STATUS_DONE:
            fics_set_new_data_callback(fics_ndcb_update_from_server);
            strcpy(global.view.scratch_buffer, FICS_CMD_PLAY);
            strcat(global.view.scratch_buffer, fics_data.game_number_str);
            plat_net_send(global.view.scratch_buffer);
            // Asking for the game - may not start though so re-enable the menu
            ui_in_game_menu.menu_items[UI_MENU_INGAME_NEW].item_state = MENU_STATE_ENABLED;
        break;

        case FS_STATUS_NO_MATCH:
        case FS_STATUS_ERROR:
            fics_set_new_data_callback(fics_ndcb_update_from_server);
            fics_play(true);
        break;

        default:
        break;
    }
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
            if (*parse_point == '\x49') { // 'I'
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

            // '1'
            global.state.includes_me = global.frame.my_relation_to_game[0] == '\x31' || global.frame.my_relation_to_game[1] == '\x31';
            global.state.my_move = global.frame.my_relation_to_game[0] == '\x31';

            if (!global.state.includes_me) {
                global.state.my_side = SIDE_WHITE;
            } else {
                // Derive my color based on whether it's my move or not
                // 'W'
                global.state.my_side = global.state.my_move ? *global.frame.color_to_move == '\x57' : *global.frame.color_to_move != '\x57';
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
            // 'W'
            fics_add_status_log(global.text.word_next, *global.frame.color_to_move == '\x57' ? global.text.side_label[SIDE_WHITE] : global.text.side_label[SIDE_BLACK]);
            fics_add_status_log(global.text.word_last, global.frame.previous_move);

            // Move past all this to see if there are more statements to parse (Game Over comes with last
            // move in all cases I observed)
            len -= (parse_point - parse_start);
            while (len && *parse_point != '\x0a') { // '\n'
                parse_point++;
                len--;
            }
        } else if (character == FICS_DATA_GAME_OVER[0] && 0 == strncmp(parse_point, FICS_DATA_GAME_OVER, (sizeof(FICS_DATA_GAME_OVER) - 1))) {
            // Game status message received
            global.view.refresh = true;
            // Skip user names
            while (len && *parse_point != '\x29') { // ')'
                parse_point++;
                len--;
            }
            // Skip ) & space
            parse_point += 2;
            len -= 2;
            if (len > 0) {
                // If it's a Creating message, it's still game-on
                if (!(*parse_point == FICS_DATA_CREATING[0] && 0 == strncmp(parse_point, FICS_DATA_CREATING, (sizeof(FICS_DATA_CREATING) - 1)))) {
                    // but if not, it's a game over message
                    global.state.game_active = false;
                }
                // Whatever message, show it
                fics_format_stats_message(parse_point, len, '\x7d'); // '}'
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
            while (len && *parse_point != '\x0a') { // '\n'
                len--;
                parse_point++;
            }
            if (*parse_point == '\x0a') {
                fics_format_stats_message(parse_start, parse_point - parse_start, '\x0a');
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
    if (ui_pregame_menu_options_menu.menu_items[UI_LOGIN_OPTIONS_REGISTERED].selected == 1 &&
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
    if (use_seek) {
        ui_in_game_menu.menu_items[UI_MENU_INGAME_STOP_SEEK].item_state = MENU_STATE_ENABLED;
        if (!(global.view.mc.df & MENU_DRAW_HIDDEN)) {
            global.view.mc.df = MENU_DRAW_REDRAW;
        }
        // 'u' 'i' 'r'
        if (global.ui.my_game_type[1] == '\x75' || global.ui.my_game_type[1] == '\x69' || global.ui.my_game_type[1] == '\x72') {
            strcpy(&global.setup.seek_cmd[5], global.ui.my_game_type);
            plat_net_send(global.setup.seek_cmd);
        }
    } else {
        // Init the search cache
        memset(&fics_data, 0, sizeof(fics_data));
        fics_data.rating_delta = -1;
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

#ifdef __ATARIXL__
#pragma code-name(pop)
#endif
