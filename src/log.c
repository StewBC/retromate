/*
 *  log.c
 *  RetroMate
 *
 *  By S. Wessels, 2025.
 *  This is free and unencumbered software released into the public domain.
 *
 */

#include <stdlib.h> // malloc/free
#include <string.h>

#include "global.h"

/*-----------------------------------------------------------------------*/
// Like memchr, but finds any character in `delims`
static const char *log_mempbrk(const char *buf, int len, const char *delims) {
    const char *d;
    int i = 0;
    for (; i < len; ++i) {
        for (d = delims; *d; ++d) {
            if (buf[i] == *d) {
                return buf + i;
            }
        }
    }
    return NULL;
}

/*-----------------------------------------------------------------------*/
static inline void log_advance_line(tLog *log) {
    if (++log->head >= log->rows) {
        log->head = 0;
        log->dest_ptr = log->buffer;
    } else {
        log->dest_ptr += log->cols;
    }
    if (log->size < log->rows) {
        log->size++;
    }
}

/*-----------------------------------------------------------------------*/
void log_clear(tLog *log) {
    log->head = log->size = 0;
    log->dest_ptr = log->buffer;
    log->modified = false;
}

/*-----------------------------------------------------------------------*/
void log_init(tLog *log, uint8_t width, uint8_t height) {
    log->cols = width;
    log->rows = height;
    log->buffer_size = width * height;
    log->buffer = plat_core_log_malloc(log->buffer_size);
    log->dest_ptr = log->buffer;
    if (!log->buffer) {
        // If you can't get the mem for a log, then you can only quit
        plat_core_exit();
    }
    memset(log->buffer, ' ', log->buffer_size);
}

/*-----------------------------------------------------------------------*/
void log_add_line(tLog *log, const char *text, int text_len) {
    int remaining, chunk_len, line_len;
    const char *p = text;
    const char *line_start;
    const char *newline;

    remaining = (text_len < 0) ? strlen(text) : text_len;
    if (log->clip) {
        remaining = remaining > log->cols ? log->cols : remaining;
    }

    while (remaining > 0) {
        // Determine the end of the current line (up to \n or end of string)
        line_start = p;
        newline = log_mempbrk(p, remaining, "\x0d\x0a");
        if (newline) {
            uint8_t skip = 1;
            char nl = *newline;
            line_len = newline - p;

            // If there's a following opposite newline (\n\r or \r\n), skip both
            if ((newline + 1 < p + remaining) &&
                    ((nl == '\x0a' && newline[1] == '\x0d') ||
                     (nl == '\x0d' && newline[1] == '\x0a'))) {
                skip = 2;
            }

            remaining -= (newline - p + skip);
            p = newline + skip;
        } else {
            line_len = remaining;
            remaining = 0;
        }

        if (line_len == 0) {
            if (line_len == 0) {
                memset(log->dest_ptr, ' ', log->cols);
            }
            log_advance_line(log);
        } else {
            while (line_len > 0) {
                chunk_len = (line_len > log->cols) ? log->cols : line_len;
                plat_core_copy_ascii_to_display(log->dest_ptr, line_start, chunk_len);

                if (chunk_len < log->cols) {
                    memset(log->dest_ptr + chunk_len, ' ', log->cols - chunk_len);
                }
                line_start += chunk_len;
                line_len -= chunk_len;
                log_advance_line(log);
            }
        }
    }
    log->modified = true;
}

/*-----------------------------------------------------------------------*/
void log_shutdown(tLog *log) {
    plat_core_log_free_mem(log->buffer);
    memset(log, 0, sizeof(tLog));
}
