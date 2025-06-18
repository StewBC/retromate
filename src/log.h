/*
 *  log.h
 *  RetroMate
 *
 *  By S. Wessels, 2025.
 *  This is free and unencumbered software released into the public domain.
 *
 */

#ifndef LOG_H
#define LOG_H

typedef struct _tLog {
    char *buffer;               // Log data storage
    unsigned int buffer_size;   // Size of buffer in bytes
    uint8_t head;               // First line
    uint8_t size;               // How many lines added
    char *dest_ptr;             // Ptr in buffer to head and x
    uint8_t cols;               // How wide is the log
    uint8_t rows;               // How many rows before wrapping
    bool modified;              // log_add_* sets to 1
    bool clip;                  // log_add_line copies onlt cols chars
} tLog;

void log_add_line(tLog *log, const char *text, int text_len);
void log_clear(tLog *log);
void log_init(tLog *log, uint8_t width, uint8_t height);
void log_shutdown(tLog *log);

#endif // LOG_H
