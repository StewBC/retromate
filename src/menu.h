/*
 *  menu.h
 *  RetroMate
 *
 *  By S. Wessels, 2025.
 *  This is free and unencumbered software released into the public domain.
 *
 */

#ifndef MENU_H
#define MENU_H

#include <stdint.h>

// When a menu tick has nothing to say
#define MENU_SELECT_BACK    0xFE
#define MENU_SELECT_NONE    0xFF

// Forward declarations
typedef struct _menu menu_t;

// Callback function pointer signature
typedef uint8_t (*menu_callback_t)(menu_t *m, void *data);

enum {
    MENU_COLOR_BACKGROUND,
    MENU_COLOR_FRAME,
    MENU_COLOR_TITLE,
    MENU_COLOR_ITEM,
    MENU_COLOR_CYCLE,
    MENU_COLOR_CALLBACK,
    MENU_COLOR_SUBMENU,
    MENU_COLOR_SELECTED,
    MENU_COLOR_DISABLED,
};

typedef enum {
    MENU_ITEM_STATIC,
    MENU_ITEM_CYCLE,
    MENU_ITEM_SUBMENU,
    MENU_ITEM_CALLBACK,
    MENU_ITEM_BACKUP,
} item_type_t;

typedef enum {
    MENU_STATE_DISABLED,
    MENU_STATE_ENABLED,
    MENU_STATE_HIDDEN,
} item_state_t;

typedef enum {
    MENU_DRAW               = 1,
    MENU_DRAW_ERASE         = 4,
    MENU_DRAW_REDRAW        = 5,
    MENU_DRAW_ITEM_DRAW     = 8,
    MENU_DRAW_ITEM_SELECT   = 16,
    MENU_DRAW_HIDDEN        = 32,
} menu_draw_t;

enum {
    MENU_ITEM_DRAW_ACTION_ONLY,
    MENU_ITEM_DRAW
};

// A menu consists of one or more menu items
typedef struct {
    char *item_name;             // menu item name shown in meny
    item_type_t type;            // how the item behaves when selected
    item_state_t item_state;     // enabled, disabled, hidden
    // The next 3 items should be in a union with the 3 that follow them
    // but cc65 can't handle the designated initializers (C99 maybe?)
    uint8_t num_selections;      // ITEM_CYCLE through this number of options
    char **selections;           // What the cycle items are called (shown on screen)
    uint8_t selected;            // Whcih cycle item is active
    // These three should overlap the last 3 but it's not a big deal
    char *edit_target;           // Name of a field to show for callback and sub-menu if set
    uint8_t edit_maxlen;         // Max chars edit_target can handle
    uint8_t filter;              // ALPHA, ALNUM, PRINTABLE
    menu_t *submenu;             // Embedded menu, or if callback == input_text_callback &&
    // filter == FILTER_NUM, you can pass an int* (cast as menu_t*)
    // to be set as *(int*) = atoi(edit_target) by input_text_callback
    menu_callback_t callback;    // ITEM_CALLBACK function to call when selected
} menu_item_t;

// A menu has a title, some number of menu times and knows which item is currently selected
struct _menu {
    char *title;                 // This menu's title
    uint8_t num_items;           // How many menu items
    menu_item_t *menu_items;     // The item menu_item's themselves
    uint8_t item_spacing;        // Extra spaces between items
    uint8_t selected_item;       // Currently selected item from menu_items
    // menu_callback_t item_change; // If set, called when the selection changes
    menu_t *parent_menu;         // Where back goes
};

typedef struct _menu_cache {
    menu_t *m;          // active menu
    menu_t *nm;         // next active menu (0 unless poping a menu)
    menu_draw_t df;     // draw_flags
    uint8_t x;          // menu's x
    uint8_t y;          // menu's y
    uint8_t w;          // menu's w
    uint8_t h;          // menu's h
    uint8_t tx;         // x of Title
    uint8_t ix;         // x of menu items
    uint8_t ax;         // x of action items
    uint8_t sy;         // selected item's y
    uint8_t ix_max;     // max length of all items
    uint8_t ax_max;     // max length of all actions
    uint8_t is;         // menu's item spacing
    uint8_t iy;         // working y
} menu_cache_t;

// Prep menu_tick for the menu it will tick over
void menu_set(menu_t *m);
// Ticks the menu system (non-blocking call in a loop)
uint8_t menu_tick();

// These are really for internal use but the ui also needs then

// Show an item or its action depending on the flags
void menu_show_item(menu_item_t *item, uint8_t draw_flag);
// See the offset of this item in the visible list
uint8_t menu_count_active(uint8_t index);

#endif // MENU_H
