/*******************************************************
 usbip-server - a platform for USB device prototyping

 Fredrik Andersson
 Copyright 2024-2025, All Rights Reserved.

 This software may be used by anyone for any reason so
 long as the copyright notice in the source files
 remains intact.

 code repository located at:
        http://github.com/freand76/usbip-server
********************************************************/

#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include "tcpip_server.h"

#define ACCEPT_TIMEOUT_US (500000)
#define READ_TIMEOUT_US (500000)

/* Detect clients that vanish without closing the connection */
#define KEEPALIVE_IDLE_S (10)
#define KEEPALIVE_INTERVAL_S (5)
#define KEEPALIVE_COUNT (3)

static int server_socket = -1;
static int client_socket = -1;
static volatile sig_atomic_t stop_server = false;

bool tcpip_server_open(int port) {
    struct sockaddr_in addr;

    /* Ignore signal SIGPIPE, Broken pipe */
    signal(SIGPIPE, SIG_IGN);

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return false;
    }

    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        tcpip_server_close();
        return false;
    }

    if (listen(server_socket, 10) < 0) {
        tcpip_server_close();
        return false;
    }

    return true;
}

void tcpip_server_close(void) {
    if (server_socket >= 0) {
        close(server_socket);
        server_socket = -1;
    }
}

void tcpip_server_stop(void) {
    stop_server = true;
}

void tcpip_server_close_client(void) {
    if (client_socket >= 0) {
        close(client_socket);
        client_socket = -1;
    }
}

bool tcpip_server_wait_for_client(void) {
    bool status;
    int result;
    struct timeval tv = {0};
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(server_socket, &rfds);

    /* Blocking wait for accept */
    tv.tv_usec = ACCEPT_TIMEOUT_US;

    result = select(server_socket + 1, &rfds, (fd_set *)0, (fd_set *)0, &tv);
    status = (result > 0);

    if (status) {
        client_socket = accept(server_socket, NULL, NULL);
        status = (client_socket > 0);
    }

    if (status) {
        tv.tv_usec = READ_TIMEOUT_US;
        setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv);
        /* Replies are written in parts, don't let Nagle hold them back waiting for an ACK */
        setsockopt(client_socket, IPPROTO_TCP, TCP_NODELAY, &(int){1}, sizeof(int));
        setsockopt(client_socket, SOL_SOCKET, SO_KEEPALIVE, &(int){1}, sizeof(int));
        setsockopt(client_socket, IPPROTO_TCP, TCP_KEEPIDLE, &(int){KEEPALIVE_IDLE_S}, sizeof(int));
        setsockopt(client_socket, IPPROTO_TCP, TCP_KEEPINTVL, &(int){KEEPALIVE_INTERVAL_S},
                   sizeof(int));
        setsockopt(client_socket, IPPROTO_TCP, TCP_KEEPCNT, &(int){KEEPALIVE_COUNT}, sizeof(int));
    }
    return status;
}

bool tcpip_server_wait_for_data(int wake_fd) {
    struct pollfd fds[2] = {
        {.fd = client_socket, .events = POLLIN},
        {.fd = wake_fd, .events = POLLIN},
    };

    if (stop_server || (poll(fds, 2, READ_TIMEOUT_US / 1000) <= 0)) {
        return false;
    }
    /* Errors and hang-ups also count, the following read will report them */
    return (fds[0].revents != 0);
}

bool tcpip_server_read(void *buffer, size_t read_size) {
    size_t length = 0;
    char *char_buffer = (char *)buffer;
    while (!stop_server && (length < read_size)) {
        int res = read(client_socket, &char_buffer[length], read_size - length);
        if ((res == -1) && ((errno == EAGAIN) || (errno == EINTR))) {
            /* Timeout or signal, an idle client is not a lost client */
            continue;
        } else if (res > 0) {
            length += res;
        } else {
            break;
        }
    }

    return (length == read_size);
}

bool tcpip_server_write(void *buffer, size_t write_size) {
    size_t length = 0;
    char *char_buffer = (char *)buffer;
    while (!stop_server && (length < write_size)) {
        int res = write(client_socket, &char_buffer[length], write_size - length);
        if ((res == -1) && (errno == EAGAIN)) {
            /* Timeout */
            return false;
        } else if (res > 0) {
            length += res;
        } else {
            break;
        }
    }
    return (length == write_size);
}
