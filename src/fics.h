/*
 *  fics.h
 *  RetroMate
 *
 *  By S. Wessels, 2025.
 *  This is free and unencumbered software released into the public domain.
 *
 */

#ifndef _FICS_H_
#define _FICS_H_

typedef void(*fics_match_callback_t)(const char *buf, int len, const char *match);
typedef void(*fics_new_data_callback_t)(const char *buf, int len);

void fics_init(void);
uint8_t fics_letter_to_piece(char letter);
void fics_play(bool use_seek);
void fics_set_new_data_callback(fics_new_data_callback_t callback);
void fics_set_trigger_callback(const char *text, fics_match_callback_t callback);
void fics_shutdown(void);
void fics_tcp_recv(const unsigned char *buf, int len);

#endif //_FICS_H_
