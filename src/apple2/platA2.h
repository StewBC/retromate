/*
 *  platA2.h
 *  RetroMate
 *
 *  By S. Wessels and O. Schmidt, 2025.
 *  This is free and unencumbered software released into the public domain.
 *
 */

#ifndef _PLATA2_H_
#define _PLATA2_H_

/*-----------------------------------------------------------------------*/
#define CLR80COL    0xC001
#define SET80COL    0xC001
#define TXTPAGE1    0xC054
#define TXTPAGE2    0xC055


/*-----------------------------------------------------------------------*/
// These are text based coordinates
#define SCREEN_TEXT_WIDTH       40
#define SCREEN_TEXT_HEIGHT      24
#define SQUARE_TEXT_WIDTH       3

// These are graphical coordinates
#define CHARACTER_WIDTH         8
#define CHARACTER_HEIGHT        8
#define SCREEN_DISPLAY_HEIGHT   (SCREEN_TEXT_HEIGHT * CHARACTER_HEIGHT)
#define SQUARE_DISPLAY_HEIGHT   22
#define BOARD_DISPLAY_HEIGHT    (SQUARE_DISPLAY_HEIGHT * 8)

#define ROP_CONST(val)          0xA980|(val)
#define ROP_BLACK               0xA980
#define ROP_WHITE               0xA9FF
#define ROP_XOR(val)            0x4900|(val)
#define ROP_CPY                 0x4980
#define ROP_INV                 0x49FF
#define ROP_AND(val)            0x2900|(val)

extern char hires_char_set[96][CHARACTER_HEIGHT];
extern char hires_pieces[6][2][SQUARE_TEXT_WIDTH * SQUARE_DISPLAY_HEIGHT];

void hires_init(void);
void hires_done(void);
void hires_text(char flag);
void hires_draw(char xpos,    char ypos,
                char xsize,   char ysize,
                unsigned rop, char *src);
void hires_mask(char xpos,    char ypos,
                char xsize,   char ysize,
                unsigned rop);


typedef struct _apple2 {
    char rop_line[2][7];
    char rop_color[2][2];
    char **help_text[2];
    uint8_t *help_text_len[2];
    uint8_t help_text_num_lines[2];
    uint8_t terminal_display_width;
    char send_buffer[80];
    char terminal_log_buffer[80 * 23];
    char status_log_buffer[13 * 24];
} apple2_t;

extern apple2_t apple2;

#endif //_PLATA2_H_
