/*
 *  usrinput.h
 *  RetroMate
 *
 *  By S. Wessels, 2025.
 *  This is free and unencumbered software released into the public domain.
 *
 */

#ifndef INPUT_H
#define INPUT_H

typedef enum {
    FILTER_ALLOW_ALL,
    FILTER_PRINTABLE,
    FILTER_ALNUM,
    FILTER_NUM
} Filter;

void input_text(char *buffer, uint8_t maxlen, uint8_t filter);
uint8_t input_text_callback(menu_t *m, void *data);

#endif // INPUT_H