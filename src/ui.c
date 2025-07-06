/*
 *  ui.c
 *  RetroMate
 *
 *  By S. Wessels, 2025.
 *  This is free and unencumbered software released into the public domain.
 *
 */

#include <stdlib.h> // atoi
#include <string.h>

#include "global.h"

// In game menu cycle selection
char *ui_game_types[GAME_TYPE_COUNT] = {"standard", "blitz", "lightning", "untimed", "crazyhouse", "wild", "suicide"};
char *ui_game_types_ascii[GAME_TYPE_COUNT] = {
    "\x73\x74\x61\x6e\x64\x61\x72\x64"        , // standard
    "\x62\x6c\x69\x74\x7a"                    , // blitz
    "\x6c\x69\x67\x68\x74\x6e\x69\x6e\x67"    , // lightning
    "\x75\x6e\x74\x69\x6d\x65\x64"            , // untimed
    "\x63\x72\x61\x7a\x79\x68\x6f\x75\x73\x65", // crazyhouse
    "\x77\x69\x6c\x64"                        , // wild
    "\x73\x75\x69\x63\x69\x64\x65"              // suicide
};
char *ui_game_start_lengths[5] = {"15", "5", "2", "0", "3" };
char *ui_game_increments[5] = {"0", "2", "2", "0", "0" };
char *wild_variants[] = {"wild0", "wild1", "wild2", "wild3", "wild4", "wild5", "wild8", "wild8a", "wild fr"};
char *wild_variants_ascii[] = {
    "\x77\x69\x6c\x64\x30"        , // wild0
    "\x77\x69\x6c\x64\x31"        , // wild1
    "\x77\x69\x6c\x64\x32"        , // wild2
    "\x77\x69\x6c\x64\x33"        , // wild3
    "\x77\x69\x6c\x64\x34"        , // wild4
    "\x77\x69\x6c\x64\x35"        , // wild5
    "\x77\x69\x6c\x64\x38"        , // wild8
    "\x77\x69\x6c\x64\x38\x61"    , // wild8a
    "\x77\x69\x6c\x64\x20\x66\x72"  // wild fr
};
char *ui_yes_no_toggle[] = { "No", "Yes"};
// SQW switch to a name change, one action
// char *ui_stop_actions[] = {"Resign", "Unobserve", "Stop Seek"};

// For matching to rated or unrated games
char ui_rating_type[2] = {'u', 'r'};
char ui_rating_type_ascii[2] = {'\x75', '\x72'};
char *ui_rating_code[2] = {"0", "1"};
// FICS variables to change the users' time and rating deired variables
char *ui_variable[] = {"time ", "inc ", "availmin ", "availmax ", "rated "};

//----------------------------------------------------------------------------
static uint8_t ui_set_rating_callback(menu_t *m, void *data) {
    menu_item_t *item = (menu_item_t *)data;
    uint8_t variable = item == &ui_settings_menu_items[UI_SETTINGS_MINRATINGMATCH] ? 0 : 1;
    input_text_callback(m, item);
    global.view.mc.iy += menu_count_active(UI_SETTINGS_MINRATINGMATCH + variable);
    ui_set_item_target(item, UI_VARIABLE_AVAILMIN + variable, item->edit_target);
    return MENU_DRAW_REDRAW;   // Don't change the flags
}

//----------------------------------------------------------------------------
static uint8_t ui_set_wild_type_callback(menu_t *m, void *data) {
    menu_item_t *item = (menu_item_t *)data;
    uint8_t selected = item->selected;
    UNUSED(m);

    global.ui.my_game_type = wild_variants_ascii[selected];
    return global.view.mc.df;
}

//----------------------------------------------------------------------------
static uint8_t ui_set_game_type_callback(menu_t *m, void *data) {
    uint8_t retval = global.view.mc.df;
    menu_item_t *item = (menu_item_t *)data;
    uint8_t selected = item->selected;
    UNUSED(m);

    global.ui.my_game_type = ui_game_types_ascii[selected];
    if (selected == GAME_TYPE_WILD) {
        m->menu_items[UI_SETTINGS_WILD_VARIANTS].item_state = MENU_STATE_ENABLED;
        ui_set_wild_type_callback(m, &m->menu_items[UI_SETTINGS_WILD_VARIANTS]);
        retval = MENU_DRAW_REDRAW;
    } else if (selected == GAME_TYPE_WILD + 1) {
        m->menu_items[UI_SETTINGS_WILD_VARIANTS].item_state = MENU_STATE_HIDDEN;
        retval = MENU_DRAW_REDRAW;
    }
    if (selected > 4) {
        selected = 4;
    }
    global.view.mc.iy += menu_count_active(UI_SETTINGS_START_TIME) - 1;
    ui_set_item_target(&m->menu_items[UI_SETTINGS_START_TIME], UI_VARIABLE_TIME, ui_game_start_lengths[selected]);
    global.view.mc.iy++;
    ui_set_item_target(&m->menu_items[UI_SETTINGS_INCREMENTALTIME], UI_VARIABLE_INC, ui_game_increments[selected]);
    return retval;   // Might redraw if wild selected/unselected
}

//----------------------------------------------------------------------------
static uint8_t ui_toggle_rated_callback(menu_t *m, void *data) {
    menu_item_t *item = (menu_item_t *)data;
    UNUSED(m);

    // for sought
    global.ui.my_rating_type[0] = ui_rating_type_ascii[item->selected];
    // for seek
    strcpy(global.view.scratch_buffer, "set ");
    strcat(global.view.scratch_buffer, ui_variable[4]);
    strcat(global.view.scratch_buffer, ui_rating_code[item->selected]);
    plat_net_send(global.view.scratch_buffer);
    return global.view.mc.df; // No draw needed
}

//----------------------------------------------------------------------------
static uint8_t ui_toggle_registerd_callback(menu_t *m, void *data) {
    menu_item_t *item = (menu_item_t *)data;
    m->menu_items[UI_LOGIN_OPTIONS_PASSWORD].item_state = MENU_STATE_HIDDEN - item->selected;
    ui_settings_menu_items[UI_SETTINGS_RATED].item_state = MENU_STATE_HIDDEN - item->selected;
    return MENU_DRAW_REDRAW; // Force a redraw since the menu size changes
}

//----------------------------------------------------------------------------
static uint8_t ui_toggle_sought_callback(menu_t *m, void *data) {
    menu_item_t *item = (menu_item_t *)data;
    global.setup.use_seek = item->selected == 1;
    m->menu_items[UI_SETTINGS_START_TIME].item_state = MENU_STATE_ENABLED + item->selected;
    m->menu_items[UI_SETTINGS_INCREMENTALTIME].item_state = MENU_STATE_ENABLED + item->selected;
    return MENU_DRAW_REDRAW; // Force a redraw since the menu size changes
}

//----------------------------------------------------------------------------
#include <stdio.h>
void ui_set_item_target(menu_item_t *item, uint8_t variable, char *value_str) {
    if(item->edit_target != value_str) {
        // Don't attempt to do an overlapped copy ie strcpy(ptr, ptr)
        strcpy(item->edit_target, value_str);
    }
    *(int *)item->submenu = atoi(value_str);
    // Redraw the item only because it will absolutely fit in the menu
    menu_show_item(item, MENU_ITEM_DRAW_ACTION_ONLY);
    strcpy(global.view.scratch_buffer, "set ");
    strcat(global.view.scratch_buffer, ui_variable[variable]);
    strcat(global.view.scratch_buffer, value_str);
    plat_net_send(global.view.scratch_buffer);
}

//----------------------------------------------------------------------------
// Pre-game menus

// Pre-game Menu Submenu: Login Options
menu_item_t ui_pregame_menu_options_menu_items[] = {
    { "Username", MENU_ITEM_CALLBACK, MENU_STATE_ENABLED, 0, 0, 0, global.ui.user_name, sizeof(global.ui.user_name), FILTER_ALNUM, NULL, input_text_callback},
    { "Password", MENU_ITEM_CALLBACK, MENU_STATE_HIDDEN, 0, 0, 0, global.ui.user_password, sizeof(global.ui.user_password), FILTER_PRINTABLE, NULL, input_text_callback},
    { "Registered", MENU_ITEM_CYCLE, MENU_STATE_ENABLED, AS(ui_yes_no_toggle), ui_yes_no_toggle, 0, NULL, 0, 0, NULL, ui_toggle_registerd_callback},
    { "Host", MENU_ITEM_CALLBACK, MENU_STATE_ENABLED, 0, 0, 0, global.ui.server_name, sizeof(global.ui.server_name), FILTER_PRINTABLE, NULL, input_text_callback},
    { "Port", MENU_ITEM_CALLBACK, MENU_STATE_ENABLED, 0, 0, 0, global.ui.server_port_str, sizeof(global.ui.server_port_str), FILTER_NUM, (menu_t *) &global.ui.server_port, input_text_callback},
    { "Back", MENU_ITEM_BACKUP, MENU_STATE_ENABLED, 0, 0, 0, NULL, 0, 0, NULL, NULL},
};
menu_t ui_pregame_menu_options_menu = {
    "Login Options", AS(ui_pregame_menu_options_menu_items), ui_pregame_menu_options_menu_items, 0, 0, NULL
};

// Pre-game Menu
menu_item_t ui_pregame_menu_items[] = {
    { "Play Online", MENU_ITEM_STATIC, MENU_STATE_ENABLED, 0, 0, 0, NULL, 0, 0, NULL, NULL},
    { "Login Options", MENU_ITEM_SUBMENU, MENU_STATE_ENABLED, 0, 0, 0, NULL, 0, 0, &ui_pregame_menu_options_menu, NULL},
    { "Quit", MENU_ITEM_BACKUP, MENU_STATE_ENABLED, 0, 0, 0, NULL, 0, 0, NULL, NULL},
};
menu_t ui_pregame_menu = {
    "RetroMate", AS(ui_pregame_menu_items), ui_pregame_menu_items, 0, 0, NULL
};

//----------------------------------------------------------------------------
// In-game menus
// In-game Menu Submenu: Game Settings
menu_item_t ui_settings_menu_items[] = {
    { "Game Type", MENU_ITEM_CYCLE, MENU_STATE_ENABLED, AS(ui_game_types), ui_game_types, 0, NULL, 0, 0, NULL, ui_set_game_type_callback},
    { "Wild Variant", MENU_ITEM_CYCLE, MENU_STATE_HIDDEN, AS(wild_variants), wild_variants, 0, NULL, 0, 0, NULL, ui_set_wild_type_callback},
    { "Rated", MENU_ITEM_CYCLE, MENU_STATE_HIDDEN, AS(ui_yes_no_toggle), ui_yes_no_toggle, 0, NULL, 0, 0, NULL, ui_toggle_rated_callback},
    { "Use Sought", MENU_ITEM_CYCLE, MENU_STATE_ENABLED, AS(ui_yes_no_toggle), ui_yes_no_toggle, 1, NULL, 0, 0, NULL, ui_toggle_sought_callback},
    { "Start Time", MENU_ITEM_CALLBACK, MENU_STATE_HIDDEN, 0, 0, 0, global.setup.starting_time_srt, sizeof(global.setup.starting_time_srt), FILTER_NUM, (menu_t *) &global.setup.starting_time, input_text_callback},
    { "Incremental Time", MENU_ITEM_CALLBACK, MENU_STATE_HIDDEN, 0, 0, 0, global.setup.incremental_time_str, sizeof(global.setup.incremental_time_str), FILTER_NUM, (menu_t *) &global.setup.incremental_time, input_text_callback},
    { "Min Rating Match", MENU_ITEM_CALLBACK, MENU_STATE_HIDDEN, 0, 0, 0, global.setup.min_rating_str, sizeof(global.setup.min_rating_str), FILTER_NUM, (menu_t *) &global.setup.min_rating, ui_set_rating_callback},
    { "Max Rating Match", MENU_ITEM_CALLBACK, MENU_STATE_HIDDEN, 0, 0, 0, global.setup.max_rating_str, sizeof(global.setup.max_rating_str), FILTER_NUM, (menu_t *) &global.setup.max_rating, ui_set_rating_callback},
    { "Back", MENU_ITEM_BACKUP, MENU_STATE_ENABLED, 0, 0, 0, NULL, 0, 0, NULL, NULL},
};
menu_t ui_settings_menu = {
    "Game Settings", AS(ui_settings_menu_items), ui_settings_menu_items, 0, 0, NULL
};

// In-Game Menu
menu_item_t ui_in_game_menu_items[] = {
    { "New Game", MENU_ITEM_STATIC, MENU_STATE_ENABLED, 0, 0, 0, NULL, 0, 0, NULL, NULL},
    { "Resign", MENU_ITEM_STATIC, MENU_STATE_HIDDEN, 0, 0, 0, NULL, 0, 0, NULL, NULL},
    { "Unobserve", MENU_ITEM_STATIC, MENU_STATE_HIDDEN, 0, 0, 0, NULL, 0, 0, NULL, NULL},
    { "Stop Seek", MENU_ITEM_STATIC, MENU_STATE_HIDDEN, 0, 0, 0, NULL, 0, 0, NULL, NULL},
    { "Game Settings", MENU_ITEM_SUBMENU, MENU_STATE_ENABLED, 0, 0, 0, NULL, 0, 0, &ui_settings_menu, NULL},
    { "View Terminal", MENU_ITEM_STATIC, MENU_STATE_ENABLED, 0, 0, 0, NULL, 0, 0, NULL, NULL},
    { "Hide Menu", MENU_ITEM_BACKUP, MENU_STATE_ENABLED, 0, 0, 0, NULL, 0, 0, NULL, NULL},
    { "Logout", MENU_ITEM_STATIC, MENU_STATE_ENABLED, 0, 0, 0, NULL, 0, 0, NULL, NULL},
};
menu_t ui_in_game_menu = {
    "RetroMate (Online)", AS(ui_in_game_menu_items), ui_in_game_menu_items, 0, 0, NULL
};
