/*
 *  platSDL2net.c
 *  RetroMate
 *
 *  By S. Wessels, 2025.
 *  This is free and unencumbered software released into the public domain.
 *
 */

#ifndef _WIN32
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <string.h>
#include <unistd.h>
#else
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#include "../global.h"

#include "platSDL2.h"

/*-----------------------------------------------------------------------*/
void plat_net_init() {
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        app_error(true, "WSAStartup failed");
    }
#endif
}

/*-----------------------------------------------------------------------*/
void plat_net_connect(const char *server_name, int server_port) {
    struct sockaddr_in serv_addr;
    struct hostent *server;

    log_add_line(&global.view.terminal, "Opening a socket", -1);
    plat_draw_log(&global.view.terminal, 0, 0, false);
    plat_draw_update();
    sdl.sockfd = socket(AF_INET, SOCK_STREAM, 0);
#ifdef _WIN32
    if (sdl.sockfd == INVALID_SOCKET) {
#else
    if (sdl.sockfd < 0) {
#endif
        app_error(true, "Error opening socket");
        return;
    }

    log_add_line(&global.view.terminal, "Resolving server name", -1);
    plat_draw_log(&global.view.terminal, 0, 0, false);
    plat_draw_update();

    server = gethostbyname(server_name);
    if (!server) {
        app_error(false, "No such server_name");
        return;
    }

    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
    serv_addr.sin_port = htons(server_port);

    log_add_line(&global.view.terminal, "Connecting to server", -1);
    plat_draw_log(&global.view.terminal, 0, 0, false);
    plat_draw_update();
    if (connect(sdl.sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        app_error(false, "Socket error connecting");
        return;
    }

    // Set non-blocking
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(sdl.sockfd, FIONBIO, &mode);
#else
    int flags = fcntl(sdl.sockfd, F_GETFL, 0);
    fcntl(sdl.sockfd, F_SETFL, flags | O_NONBLOCK);
#endif

    sdl.receive_callback = fics_tcp_recv;
    log_add_line(&global.view.terminal, "Logging in, please be patient", -1);
    plat_draw_log(&global.view.terminal, 0, 0, false);
    plat_draw_update();
}

/*-----------------------------------------------------------------------*/
void plat_net_disconnect() {
#ifdef _WIN32
    if (sdl.sockfd != INVALID_SOCKET) {
        closesocket(sdl.sockfd);
        sdl.sockfd = INVALID_SOCKET;
    }
#else
    if (sdl.sockfd >= 0) {
        close(sdl.sockfd);
        sdl.sockfd = -1;
    }
#endif
}

/*-----------------------------------------------------------------------*/
bool plat_net_update() {
#ifdef _WIN32
    if (sdl.sockfd == INVALID_SOCKET) {
#else
    if (sdl.sockfd < 0) {
#endif
        return 1;
    }

#ifdef _WIN32
    WSAPOLLFD pfd;
    pfd.fd = sdl.sockfd;
    pfd.events = POLLRDNORM;
    pfd.revents = 0;

    int result = WSAPoll(&pfd, 1, 0); // timeout = 0ms, non-blocking
#else
    struct pollfd pfd = {
        .fd = sdl.sockfd,
        .events = POLLIN
    };

    int result = poll(&pfd, 1, 0); // timeout = 0ms, non-blocking
#endif

    if (result > 0) {
#ifdef _WIN32
        if (pfd.revents & POLLRDNORM) {
#else
        if (pfd.revents & POLLIN) {
#endif
            int len = recv(sdl.sockfd, sdl.recv_buf, sizeof(sdl.recv_buf) - 1, 0);
            if (len > 0) {
                sdl.recv_buf[len] = '\0';
                if (sdl.receive_callback) {
                    sdl.receive_callback(sdl.recv_buf, len);
                }
                return 1;
            } else if (len == 0) {
                app_error(false, "Connection closed by remote");
                return 1;
            } else {
#ifdef _WIN32
                int err = WSAGetLastError();
                if (err != WSAEWOULDBLOCK && err != WSAEINPROGRESS) {
#else
                if (errno != EWOULDBLOCK && errno != EAGAIN) {
#endif
                    app_error(false, "Socket recv error");
                    return 1;

                }
            }
        }

        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
            app_error(false, "Socket error/hang-up");
            return 1;
        }
    }

    return 0;
}


/*-----------------------------------------------------------------------*/
void plat_net_send(const char *text) {
#ifdef _WIN32
    if (sdl.sockfd == INVALID_SOCKET) {
#else
    if (sdl.sockfd < 0) {
#endif
        return;
    }
    int len = strlen(text);
    log_add_line(&global.view.terminal, text, len);
    send(sdl.sockfd, text, len, 0);
    send(sdl.sockfd, "\n", 1, 0);
}

/*-----------------------------------------------------------------------*/
void plat_net_shutdown() {
    plat_net_disconnect();
#ifdef _WIN32
    WSACleanup();
#endif
}
