/*
 *  plat.h
 *  RetroMate
 *
 *  By S. Wessels, 2025.
 *  This is free and unencumbered software released into the public domain.
 *
 */

#ifndef _PLAT_H_
#define _PLAT_H_

typedef struct _tLog tLog;
typedef struct _input_event input_event_t;

// This maps menu colors to platform colors
extern uint8_t plat_mc2pc[9];

#define MOUSE_HIT_NONE  0xFF

// Core
void plat_core_active_term(bool active);
void plat_core_exit(void);
uint8_t plat_core_get_cols(void);
uint8_t plat_core_get_rows(void);
uint8_t plat_core_get_status_x(void);
void plat_core_init(void);
uint8_t plat_core_key_input(input_event_t *evt);
void plat_core_key_wait_any(void);
void plat_core_log_free_mem(char *mem);
char *plat_core_log_malloc(unsigned int size);
uint8_t plat_core_mouse_to_cursor(void);
uint8_t plat_core_mouse_to_menu_item(void);
void plat_core_shutdown(void);

// Draw
void plat_draw_background(void);
void plat_draw_board_accoutrements(void);
void plat_draw_board(void);
void plat_draw_clear_input_line(bool active);
void plat_draw_clear_statslog_area(uint8_t row);
void plat_draw_clrscr(void);
void plat_draw_highlight(uint8_t position, uint8_t color);
void plat_draw_log(tLog *log, uint8_t x, uint8_t y, bool use_color);
void plat_draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t color);
void plat_draw_set_color(uint8_t color);
void plat_draw_set_text_bg_color(uint8_t color);
void plat_draw_splash_screen(void);
void plat_draw_square(uint8_t position);
void plat_draw_text(uint8_t x, uint8_t y, const char *text, uint8_t len);
void plat_draw_update(void);

// Telnet
void plat_net_init();
void plat_net_connect(const char *server_name, int server_port);
bool plat_net_update(void);
void plat_net_send(const char *text);
void plat_net_shutdown(void);

#endif //_PLAT_H_
