/*
 *  platC64net.c
 *  RetroMate
 *
 *  By S. Wessels and O. Schmidt, 2025.
 *  This is free and unencumbered software released into the public domain.
 *
 */

#include <string.h>
#include <ip65.h>

#include "../global.h"

#include <c64.h>

/*-----------------------------------------------------------------------*/
void plat_net_init() {
    log_add_line(&global.view.terminal, "Initializing Network", -1);
    plat_draw_log(&global.view.terminal, 0, 0, false);
    if (ip65_init(ETH_INIT_DEFAULT)) {
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
    int len = strlen(text);
    log_add_line(&global.view.terminal, text, len);
    tcp_send((unsigned char *)text, len);
    tcp_send((unsigned char *)"\n", 1);
}

/*-----------------------------------------------------------------------*/
void plat_net_shutdown() {
    tcp_close();
}
