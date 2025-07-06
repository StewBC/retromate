/*
 *  ui.h
 *  RetroMate
 *
 *  By S. Wessels, 2025.
 *  This is free and unencumbered software released into the public domain.
 *
 */

#ifndef _UI_H
#define _UI_H

enum {  // Menu - ui_pregame_menu_options_menu - Login Options
    UI_LOGIN_OPTIONS_USERNAME,
    UI_LOGIN_OPTIONS_PASSWORD,
    UI_LOGIN_OPTIONS_REGISTERED,
    UI_LOGIN_OPTIONS_HOST,
    UI_LOGIN_OPTIONS_PORT,
    UI_LOGIN_OPTIONS_BACK,
};

enum {  // Menu - ui_pregame_menu - RetroMate
    UI_MENU_MAIN_PLAY,
    UI_MENU_MAIN_OPTIONS,
    UI_MENU_MAIN_QUIT,
};

enum {  // Menu - ui_settings_menu - Game Settings
    UI_SETTINGS_GAME_TYPE,
    UI_SETTINGS_WILD_VARIANTS,
    UI_SETTINGS_RATED,
    UI_SETTINGS_USE_SOUGHT,
    UI_SETTINGS_START_TIME,
    UI_SETTINGS_INCREMENTALTIME,
    UI_SETTINGS_MINRATINGMATCH,
    UI_SETTINGS_MAXRATINGMATCH,
    UI_SETTINGS_BACK,
};

enum {  // Menu - ui_in_game_menu - RetroMate (Online)
    UI_MENU_INGAME_NEW,
    UI_MENU_INGAME_RESIGN,
    UI_MENU_INGAME_UNOBSERVE,
    UI_MENU_INGAME_STOP_SEEK,
    UI_MENU_INGAME_SETTINGS,
    UI_MENU_INGAME_TERMINAL,
    UI_MENU_INGAME_BACK,
    UI_MENU_INGAME_QUIT,
};

extern menu_t ui_pregame_menu_options_menu;
extern menu_t ui_pregame_menu;
extern menu_t ui_in_game_menu;

// Everything below this point needed only for initialisation in app.c

// void ui_set_rating_target(menu_item_t *item, uint8_t variable);
// void ui_set_time_target(menu_item_t *item, uint8_t variable, char *value_str);
void ui_set_item_target(menu_item_t *item, uint8_t variable, char *value_str);
extern menu_item_t ui_settings_menu_items[];

enum {  // ui_game_types
    GAME_TYPE_STANDARD,
    GAME_TYPE_BLITZ,
    GAME_TYPE_LIGHTNING,
    GAME_TYPE_UNTIMED,
    GAME_TYPE_CRAZYHOUSE,
    GAME_TYPE_WILD,
    GAME_TYPE_SUICIDE,
    GAME_TYPE_COUNT
};

enum { // ui_variable
    UI_VARIABLE_TIME,
    UI_VARIABLE_INC,
    UI_VARIABLE_AVAILMIN,
    UI_VARIABLE_AVAILMAX
};

extern char *ui_game_types_ascii[GAME_TYPE_COUNT];
extern char *ui_game_start_lengths[5];
extern char *ui_game_increments[5];

#endif
