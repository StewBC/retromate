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

enum {
    SOUGHT_GAME_NUM,                // 0
    SOUGHT_RANKING,                 // 1
    SOUGHT_USER_NAME,               // 2
    SOUGHT_START_TIME,              // 3
    SOUGHT_INC_TIME,                // 4
    SOUGHT_RATED,                   // 5
    SOUGHT_GAME_TYPE,               // 6
    SOUGHT_START_COLOR,             // 7
    SOUGHT_RANGE,                   // 8
    SOUGHT_EXTRA,                   // 9
    SOUGHT_COUNT,                   // 10 - Number of words - all words before this
};

typedef struct _fics_data {
    char status;                                // okay, error, need more data
    char parse_state;                           // what to look for in incoming sought data
    const char *parse_point;                    // Where in the buffer the parser is looking
    int length;                                 // length left beyond parse_point to search
    const char *sought_word[SOUGHT_COUNT];      // the words found in the current "line" - start ptr
    int sought_word_len[SOUGHT_COUNT];          // character count of words in sought_words
    char game_number_str[6];                    // The sought game to play's game number
    uint16_t rating_delta;                      // Used to determine best opponent (closest rating match)
} fics_data_t;

extern fics_data_t fics_data;

#endif //_FICS_H_
