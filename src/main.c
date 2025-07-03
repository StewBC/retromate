/*
 *  main.c
 *  RetroMate
 *
 *  By S. Wessels, 2025.
 *  This is free and unencumbered software released into the public domain.
 *
 */

#include "global.h"

/*-----------------------------------------------------------------------*/
int main() {
    log_init(&global.view.terminal, 80, plat_core_get_rows() - 1);
    log_init(&global.view.info_panel, plat_core_get_cols() - plat_core_get_status_x(), plat_core_get_rows());

    plat_core_init();
    plat_net_init();

    global.view.info_panel.clip = true;
    app_set_state(APP_STATE_OFFLINE);

    while (!global.app.quit) {
        plat_core_key_input(&global.os.input_event);
        app_draw_update();
        global.app.selection = menu_tick();
        global.app.tick();
        plat_net_update();
        plat_draw_update();
    }

    plat_net_shutdown();
    log_shutdown(&global.view.info_panel);
    log_shutdown(&global.view.terminal);
    plat_core_shutdown();

    return 0;
}
