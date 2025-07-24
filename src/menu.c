/*
 *  menu.c
 *  RetroMate
 *
 *  By S. Wessels, 2025.
 *  This is free and unencumbered software released into the public domain.
 *
 */


#include <ctype.h>
#include <string.h>
#include "global.h"

/*-----------------------------------------------------------------------*/
// Thes makes code more readable.  The color mapping is done from menu colors
// to platform colors by a lookup in the platform mapping array
// The _clipped version is used to clip the action part of a menu to the max
// menu width
#define menu_draw_string(x,y,c,t)           plat_draw_set_color(plat_mc2pc[c]);plat_draw_text(x,y,t,strlen(t))
#define menu_draw_string_clipped(x,y,c,t)   plat_draw_set_color(plat_mc2pc[c]);plat_draw_text(x,y,t,MIN(global.view.mc.ax_max,strlen(t)))
#define menu_draw_rect(x,y,w,h,c)           plat_draw_rect(x,y,w,h,plat_mc2pc[c])
#define menu_draw_set_text_bg_color(c)      plat_draw_set_text_bg_color(plat_mc2pc[c])

/*-----------------------------------------------------------------------*/
// The flag is draw item, and/or item/action as selected.  the action
// is always drawn (it could cycle, or is being selected/deselcted)
// static void menu_show_item(menu_item_t *item, uint8_t draw_flag) {
void menu_show_item(menu_item_t *item, uint8_t draw_flag) {
    uint8_t color;

    // Color, or skip if hidden
    switch (item->item_state) {
        case MENU_STATE_DISABLED:
            color = MENU_COLOR_DISABLED;
            break;

        case MENU_STATE_ENABLED:
            color = MENU_COLOR_ITEM;
            break;

        default: //MENU_STATE_HIDDEN
            return;
    }

    if (draw_flag & MENU_ITEM_DRAW) {
        // Show the item text
        menu_draw_rect(global.view.mc.ix, global.view.mc.iy, global.view.mc.ix_max, 1, MENU_COLOR_BACKGROUND);
        menu_draw_string(global.view.mc.ix, global.view.mc.iy, color, item->item_name);
    }

    // Now the action (Always drawn - MENU_ITEM_DRAW_ACTION_ONLY is always implied (it's 0))
    switch (item->type) {
        case MENU_ITEM_CYCLE:
            menu_draw_rect(global.view.mc.ax, global.view.mc.iy, global.view.mc.ax_max, 1, MENU_COLOR_BACKGROUND);
            menu_draw_string_clipped(global.view.mc.ax, global.view.mc.iy, MENU_COLOR_CYCLE, item->selections[item->selected]);
            break;

        case MENU_ITEM_CALLBACK:
            if (item->edit_target) {
                // Since callbacks always redraw, this clear is not needed.  Also,
                // ui_set_item_target might change item->edit_target after menu_cache ran,
                // global.view.mc.ax_max might be 0 when coming in here...
                // menu_draw_rect(global.view.mc.ax, global.view.mc.iy, global.view.mc.ax_max, 1, MENU_COLOR_BACKGROUND);
                menu_draw_string_clipped(global.view.mc.ax, global.view.mc.iy, MENU_COLOR_CALLBACK, item->edit_target);
            }
            break;

        case MENU_ITEM_SUBMENU:
            if (item->edit_target) {
                menu_draw_string_clipped(global.view.mc.ax, global.view.mc.iy, MENU_COLOR_SUBMENU, item->edit_target);
            }
            break;

        default:
            break;
    }
}

/*-----------------------------------------------------------------------*/
uint8_t menu_count_active(uint8_t index) {
    menu_t *m = global.view.mc.m;
    uint8_t count = m->menu_items[index].item_state != MENU_STATE_HIDDEN;

    while (index) {
        index--;
        if (m->menu_items[index].item_state != MENU_STATE_HIDDEN) {
            count++;
        }
    }
    return count;
}

/*-----------------------------------------------------------------------*/
static void menu_cache(menu_t *m) {
    uint8_t i, j, length, sel = 0;
    uint8_t height = 0;

    if(m->selected_item > m->num_items || m->menu_items[m->selected_item].item_state != MENU_STATE_ENABLED) {
        m->selected_item = 0;
    }

    // Minimum menu width 1/4 of screen (or at least holds the title)
    global.view.mc.w = MAX(2 + strlen(m->title), plat_core_get_cols() >> 2);
    global.view.mc.ix_max = global.view.mc.ax_max = 0;

    for (i = 0; i < m->num_items; i++) {
        if (m->menu_items[i].item_state == MENU_STATE_HIDDEN) {
            continue;
        }
        height++;
        if (i == m->selected_item) {
            // This bakes into sel the title offset, coming here
            sel = height;
        }
        length = strlen(m->menu_items[i].item_name);
        if (length > global.view.mc.ix_max) {
            global.view.mc.ix_max = length;
        }

        switch (m->menu_items[i].type) {
            case MENU_ITEM_CYCLE:
                for (j = 0; j < m->menu_items[i].num_selections; j++) {
                    length = strlen(m->menu_items[i].selections[j]);
                    if (length > global.view.mc.ax_max) {
                        global.view.mc.ax_max = length;
                    }
                }
                break;

            case MENU_ITEM_SUBMENU:
            case MENU_ITEM_CALLBACK:
                if (m->menu_items[i].edit_target) {
                    int length = strlen(m->menu_items[i].edit_target);
                    if (length > global.view.mc.ax_max) {
                        global.view.mc.ax_max = length;
                    }
                }
                break;

            default:
                break;
        }
    }

    // 2 spaces betweem frame & item and if action, 2 between action and item
    length = 4 + global.view.mc.ix_max + (global.view.mc.ax_max ? (global.view.mc.ax_max + 2) : 0);
    if (length > plat_core_get_cols()) {
        // screen width - (2 for frames + 2 inside frame + 2 between item/action + item width)
        global.view.mc.ax_max = plat_core_get_cols() - 6 - global.view.mc.ix_max;
        length = 4 + global.view.mc.ix_max + (global.view.mc.ax_max ? (global.view.mc.ax_max + 2) : 0);
    }
    if (length > global.view.mc.w) {
        global.view.mc.w = length;
    }

    // 1 for title
    height++;
    // 2 for the frame + content
    global.view.mc.h = 2 + height;

    // Centre on screen in X
    global.view.mc.x = (plat_core_get_cols() - global.view.mc.w) / 2;
    // items drawn inside frame (frame + 1 space)
    global.view.mc.ix = global.view.mc.x + 2;
    // actions are 2 past the end of the longest item
    global.view.mc.ax = global.view.mc.ix + global.view.mc.ix_max + 2;
    // Centre on screen in Y
    global.view.mc.y = (plat_core_get_rows() - global.view.mc.h) / 2;
    // Selector is 1 below frame, spaced for offset and then item index
    global.view.mc.sy = global.view.mc.y + 1 + sel;
    // Centre the title in the menu-draw-space
    global.view.mc.tx = global.view.mc.x + (global.view.mc.w - strlen(m->title)) / 2;
}

/*-----------------------------------------------------------------------*/
static void menu_change_selection(uint8_t new) {
    menu_t *m = global.view.mc.m;
    uint8_t old = m->selected_item;
    uint8_t index;

    menu_draw_string(global.view.mc.x + 1, global.view.mc.sy, MENU_COLOR_SELECTED, " ");
    menu_draw_string(global.view.mc.x + global.view.mc.w - 2, global.view.mc.sy, MENU_COLOR_SELECTED, " ");
    index = menu_count_active(new);
    global.view.mc.sy = global.view.mc.y + 1 + index;
    menu_draw_string(global.view.mc.x + 1, global.view.mc.sy, MENU_COLOR_SELECTED, ">");
    menu_draw_string(global.view.mc.x + global.view.mc.w - 2, global.view.mc.sy, MENU_COLOR_SELECTED, "<");
    m->selected_item = new;
}

/*-----------------------------------------------------------------------*/
static void menu_next_item() {
    menu_t *m = global.view.mc.m;
    int8_t sel = m->selected_item;
    int8_t curr_sel = sel;

    do {
        if (++sel >= m->num_items) {
            sel = 0;
        }
    } while (m->menu_items[sel].item_state != MENU_STATE_ENABLED && sel != curr_sel);
    if (sel != curr_sel) {
        menu_change_selection(sel);
    }
}

/*-----------------------------------------------------------------------*/
static void menu_prev_item() {
    menu_t *m = global.view.mc.m;
    int8_t sel = m->selected_item;
    int8_t curr_sel = sel;

    do {
        sel = sel == 0 ? m->num_items - 1 : sel - 1;
    } while (m->menu_items[sel].item_state != MENU_STATE_ENABLED && sel != curr_sel);
    if (sel != curr_sel) {
        menu_change_selection(sel);
    }
}

/*-----------------------------------------------------------------------*/
void menu_set(menu_t *m) {
    if (global.view.mc.df & MENU_DRAW_ERASE) {
        // If erase is set in the cache, there's no menu on screen so just draw
        global.view.mc.df = MENU_DRAW;
    } else {
        // If the menu erase flag not set, the old menu must first be erased
        global.view.mc.df = MENU_DRAW_REDRAW;
    }
    global.view.mc.nm = m;
    global.view.mc.m = NULL;
}

/*-----------------------------------------------------------------------*/
// Ticks the menu system (non-blocking call in a loop)
uint8_t menu_tick() {
    int8_t i, sel;
    menu_t *m;

    if (global.view.mc.df & MENU_DRAW_ERASE) {
        // Use the old cache coords to erase the old menu
        plat_draw_background();
        global.view.mc.df &= ~MENU_DRAW_ERASE;
    }

    if (global.view.mc.df & MENU_DRAW_HIDDEN) {
        return MENU_SELECT_NONE;
    }

    if (!global.view.mc.m) {
        // If the menu changed to a new menu, update the cache
        m = global.view.mc.m = global.view.mc.nm;
        global.view.mc.nm = NULL;
        if (!m) {
            global.view.mc.df = MENU_DRAW_HIDDEN;
            return MENU_SELECT_NONE;
        } else {
            menu_cache(m);
        }
    } else {
        m = global.view.mc.m;
    }

    // Get the selected item
    sel = m->selected_item;

    // Reset the item Y parameter to just under the border
    global.view.mc.iy = 1 + global.view.mc.y;

    // Prep to draw something
    menu_draw_set_text_bg_color(MENU_COLOR_BACKGROUND);

    if (global.view.mc.df & MENU_DRAW) {
        // Totally draw menu.  Start with frame and blank background
        menu_draw_rect(global.view.mc.x, global.view.mc.y, global.view.mc.w, 1, MENU_COLOR_FRAME);
        menu_draw_rect(global.view.mc.x, global.view.mc.y + global.view.mc.h - 1, global.view.mc.w, 1, MENU_COLOR_FRAME);
        menu_draw_rect(global.view.mc.x, global.view.mc.y + 1, 1, global.view.mc.h - 2, MENU_COLOR_FRAME);
        menu_draw_rect(global.view.mc.x + global.view.mc.w - 1, global.view.mc.y + 1, 1, global.view.mc.h - 2, MENU_COLOR_FRAME);
        menu_draw_rect(global.view.mc.x + 1, global.view.mc.y + 1, global.view.mc.w - 2, global.view.mc.h - 2, MENU_COLOR_BACKGROUND);

        // Title
        menu_draw_string(global.view.mc.tx, global.view.mc.iy, MENU_COLOR_TITLE, m->title);

        // All the items
        for (i = 0; i < m->num_items; i++) {
            menu_item_t *item = &m->menu_items[i];
            if (item->item_state != MENU_STATE_HIDDEN) {
                global.view.mc.iy++;
                menu_show_item(item, MENU_ITEM_DRAW);
            }
        }
        global.view.mc.df = 0;
        // Show the selected item (choose new if not enabled)
        if (m->menu_items[sel].item_state != MENU_STATE_ENABLED) {
            if (sel) {
                menu_prev_item();
            } else {
                menu_next_item();
            }
        } else {
            // will erase and draw on same item - sel
            menu_change_selection(sel);
        }
    }

    if (global.os.input_event.code != INPUT_UNKNOWN) {
        // If there's any input the menu should process
        if (global.os.input_event.code == INPUT_MOUSE_MOVE || global.os.input_event.code == INPUT_MOUSE_CLICK) {
            // Mouse input
            uint8_t mouse_selected_item;
            // See if the mouse is hovering over a menu item
            if ((mouse_selected_item = plat_core_mouse_to_menu_item()) != MENU_SELECT_NONE) {
                // Now see which visible item that is
                uint8_t count = 0;
                for (i = 0; i < m->num_items; i++) {
                    if (m->menu_items[i].item_state != MENU_STATE_HIDDEN) {
                        if (count == mouse_selected_item) {
                            // See if this is not already selected, and enabled
                            if (sel != i && m->menu_items[i].item_state != MENU_STATE_DISABLED) {
                                menu_change_selection(i);
                                sel = i;
                            }
                            break;
                        }
                        count++;
                    }
                }
                if (global.os.input_event.code == INPUT_MOUSE_CLICK) {
                    global.os.input_event.code = INPUT_SELECT;
                }
            }
        }

        // Handle the input event
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
            case INPUT_QUIT:
            case INPUT_BACK:
                menu_set(m->parent_menu);
                return MENU_SELECT_BACK;

            case INPUT_UP:
            case INPUT_LEFT:
                menu_prev_item();
                break;

            case INPUT_DOWN:
            case INPUT_RIGHT:
                menu_next_item();
                break;

            case INPUT_SELECT:
                switch (m->menu_items[sel].type) {
                    case MENU_ITEM_STATIC:
                        return sel;

                    case MENU_ITEM_CYCLE:
                        if (++m->menu_items[sel].selected >= m->menu_items[sel].num_selections) {
                            m->menu_items[sel].selected = 0;
                        }
                        global.view.mc.iy = global.view.mc.sy;
                        menu_show_item(&m->menu_items[sel], MENU_ITEM_DRAW_ACTION_ONLY);
                        if (m->menu_items[sel].callback) {
                            goto callback;
                        }
                        break;

                    case MENU_ITEM_SUBMENU:
                        m->menu_items[sel].submenu->parent_menu = m;
                        menu_set(m->menu_items[sel].submenu);
                        break;

                    case MENU_ITEM_CALLBACK:
                        if (m->menu_items[sel].callback) {
callback:
                            global.view.mc.df = m->menu_items[sel].callback(m, &m->menu_items[sel]);
                            // If the item is no longer enabled, find an enabled item
                            if (m->menu_items[sel].item_state != MENU_STATE_ENABLED) {
                                menu_prev_item();
                            }
                            // If the flags call for erase, force a recalculate the sizes
                            if (global.view.mc.df & MENU_DRAW_ERASE) {
                                // But delay till after erase so old menu is properly erased
                                global.view.mc.nm = global.view.mc.m;
                                global.view.mc.m = NULL;
                            }
                        }
                        break;

                    case MENU_ITEM_BACKUP:
                        menu_set(m->parent_menu);
                        return MENU_SELECT_BACK;
                        break;
                }
                break;

            default:
                break;
        }
    }
    return MENU_SELECT_NONE;
}
