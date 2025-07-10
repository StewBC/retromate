/*
 *  platC64.h
 *  RetroMate
 *
 *  By S. Wessels and O. Schmidt, 2025.
 *  This is free and unencumbered software released into the public domain.
 *
 */

#ifndef _PLATC64_H_
#define _PLATC64_H_

/*-----------------------------------------------------------------------*/
// These are text based coordinates
#define SCREEN_TEXT_WIDTH       40
#define SCREEN_TEXT_HEIGHT      25
#define SQUARE_TEXT_WIDTH       3
#define SQUARE_TEXT_HEIGHT      3

// These are graphical coordinates
#define CHARACTER_WIDTH         8
#define CHARACTER_HEIGHT        8
#define SCREEN_DISPLAY_HEIGHT   (SCREEN_TEXT_HEIGHT * CHARACTER_HEIGHT)
#define SQUARE_DISPLAY_HEIGHT   (SQUARE_TEXT_HEIGHT * CHARACTER_HEIGHT)
#define BOARD_DISPLAY_HEIGHT    (SQUARE_DISPLAY_HEIGHT * 8)

#define VIC_BASE_RAM            (0xC000)
#define BITMAP_OFFSET           (0x0000)
#define CHARMAP_ROM             (0xD000)
#define SCREEN_RAM              ((char*)VIC_BASE_RAM + 0x2000)
#define CHARMAP_RAM             ((char*)VIC_BASE_RAM + 0x2800)
#define CHARCOLOR               ((char*)0x286)

// For keys
#define MODKEY                  (*(char*)0x28D)
#define SHIFT_KEY               1
#define COMMODORE_KEY           2
#define CONTROL_KEY             4

#define ROP_CONST(val)          0xA900|(val)
#define ROP_BLACK               0xA900
#define ROP_WHITE               0xA9FF
#define ROP_XOR(val)            0x4900|(val)
#define ROP_CPY                 0x4900
#define ROP_INV                 0x49FF
#define ROP_AND(val)            0x2900|(val)

void plat_core_hires(bool on);
void hires_draw(char xpos,    char ypos,
                char xsize,   char ysize,
                unsigned rop, char *src);
void hires_mask(char xpos,    char ypos,
                char xsize,   char ysize,
                unsigned rop);
void hires_color(char xpos,   char ypos,
                 char xsize,  char ysize,
                 char color);

/*-----------------------------------------------------------------------*/
typedef struct _c64 {
    char rop_line[2][7];
    char rop_color[2][2];
    uint32_t draw_colors;
    uint8_t terminal_display_width;
    char send_buffer[80];
    char terminal_log_buffer[80 * 24];
    char status_log_buffer[13 * 25];
} c64_t;

extern c64_t c64;
extern char hires_pieces[6][2][SQUARE_TEXT_WIDTH * SQUARE_DISPLAY_HEIGHT];

#endif //_PLATC64_H_
