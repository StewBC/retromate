/*
 *  platC64net.c
 *  RetroMate
 *
 *  By S. Wessels and O. Schmidt, 2025.
 *  This is free and unencumbered software released into the public domain.
 *
 */

#include <c64.h>
#include <string.h>

#include "../global.h"

#include "platC64.h"

/*-----------------------------------------------------------------------*/
static int plat_net_make_ascii(const char *text) {
    char i = 0;
    while (*text) {
        char c = *text;
        if(c == 0x0d) { // \r to \n
            c64.send_buffer[i++] = 0x0a;
        } else if (c < 219 && c >= 32) {    // ignore too small and big
            if(c >= 193) { // Petscii 'A-Z' to Ascii 'A-Z'
               c64.send_buffer[i++] = c & ~128;
            } else if(c < 123) { // Ignore 123 - 192
                if(c >= 91 || c < 65) {  // pass all (that remains), but 'a-z', as-is
                    c64.send_buffer[i++] = c;
                } else {    // Petscii 'a-z' to Ascii 'a-z'
                    c64.send_buffer[i++] = c | 32;
                }
            }
        }
        text++;
    }
    c64.send_buffer[i++] = '\x0a';
    return i;
}

#if USE_IP65

#include <ip65.h>

/*-----------------------------------------------------------------------*/
void plat_net_init() {
    if (ip65_init(ETH_INIT_DEFAULT)) {
        log_add_line(&global.view.terminal, "Initializing Network", -1);
        plat_draw_log(&global.view.terminal, 0, 0, false);
        app_error(true, ip65_strerror(ip65_error));
    }
}

/*-----------------------------------------------------------------------*/
void plat_net_connect(const char *server_name, int server_port) {
    uint32_t server;

    log_add_line(&global.view.terminal, "Obtaining IP address", -1);
    plat_draw_log(&global.view.terminal, 0, 0, false);
    if (dhcp_init()) {
        app_error(true, ip65_strerror(ip65_error));
    }

    log_add_line(&global.view.terminal, "Resolving Server", -1);
    plat_draw_log(&global.view.terminal, 0, 0, false);
    server = dns_resolve(server_name);
    if (!server) {
        app_error(false, ip65_strerror(ip65_error));
    }

    log_add_line(&global.view.terminal, "Connecting to server", -1);
    plat_draw_log(&global.view.terminal, 0, 0, false);
    if (tcp_connect(server, server_port, fics_tcp_recv)) {
        app_error(false, ip65_strerror(ip65_error));
    }

    log_add_line(&global.view.terminal, "Logging in, please be patient", -1);
    plat_draw_log(&global.view.terminal, 0, 0, false);
}

/*-----------------------------------------------------------------------*/
void plat_net_disconnect() {
    tcp_close();
}

/*-----------------------------------------------------------------------*/
bool plat_net_update() {
    if (ip65_process()) {
        // I am not sure what erors could be returned here
        if (ip65_error >= IP65_ERROR_PORT_IN_USE) {
            return 1;
        }
    }
    return 0;
}

/*-----------------------------------------------------------------------*/
void plat_net_send(const char *text) {
    log_add_line(&global.view.terminal, text, -1);
    tcp_send((unsigned char *)c64.send_buffer, plat_net_make_ascii(text));
}

/*-----------------------------------------------------------------------*/
void plat_net_shutdown() {
    plat_net_disconnect();
}

#elif USE_TR

// Assembly functions in swlinkC64.s
extern void sw_init(void);
extern void sw_idle(void);
extern void sw_send(uint8_t len);
extern void sw_shutdown(void);

void plat_net_init() {
    sw_init();
}

void plat_net_connect(const char *host, int port) {
    int len;
    UNUSED(port);

    strcpy(c64.send_buffer, "atdt");
    strcat(c64.send_buffer, host);
    strcat(c64.send_buffer, ":");
    strcat(c64.send_buffer, global.ui.server_port_str);
    strcat(c64.send_buffer, "\n");
    len = strlen(c64.send_buffer);
    // Don't go through plat_net_send because this isn't ascii
    sw_send(len);
}

void plat_net_disconnect() {
    plat_net_shutdown();
}

// This does not check that the head doesn't catch the tail
// It assumes the buffer is big enough, also assumes
// all data sent is <= 255 bytes
void plat_net_send(const char *text) {
    uint8_t len = plat_net_make_ascii(text);
    log_add_line(&global.view.terminal, text, -1);
    sw_send(len);
}

void plat_net_shutdown() {
    sw_shutdown();  // Reset the modem
    if(!global.app.quit) {
        // If the app isn't quitting, restart the swiftlink
        sw_init();
    }
}

#endif
