/*
 *  app.h
 *  RetroMate
 *
 *  By S. Wessels, 2025.
 *  This is free and unencumbered software released into the public domain.
 *
 */

#ifndef _APP_H
#define _APP_H

enum {
    APP_STATE_OFFLINE,
    APP_STATE_ONLINE_INIT,
    APP_STATE_ONLINE,
};

void app_draw_update(void);
void app_error(bool fatal, const char *error_text);
void app_set_state(uint8_t new_state);
void app_state_offline(void);
void app_state_online_init(void);
void app_state_online(void);
void app_terminal_active(bool active);
void app_user_input(void);

#endif // _APP_H