/*******************************************************
 usbip-server - a platform for USB device prototyping

 Fredrik Andersson
 Copyright 2024, All Rights Reserved.

 This software may be used by anyone for any reason so
 long as the copyright notice in the source files
 remains intact.

 code repository located at:
        http://github.com/freand76/usbip-server
********************************************************/

#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
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

static int server_socket = -1;
static int client_socket = -1;
static volatile bool stop_server = false;

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
    if (server_socket > 0) {
        close(server_socket);
        server_socket = -1;
    }
}

void tcpip_server_stop(void) {
    stop_server = true;
}

void tcpip_server_close_client(void) {
    if (client_socket > 0) {
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
    }
    return status;
}

bool tcpip_server_read(void *buffer, size_t read_size) {
    size_t length = 0;
    while (!stop_server && (length < read_size)) {
        int res = read(client_socket, buffer, read_size);
        if ((res == -1) && (errno == EAGAIN)) {
            /* Timeout */
        } else if (res > 0) {
            length += res;
        } else {
            break;
        }
    }

    return (length == read_size);
}

bool tcpip_server_write(void *buffer, size_t write_size) {
    int res = write(client_socket, buffer, write_size);
    size_t length = 0;
    if (res > 0) {
        length = res;
    }
    return (length == write_size);
}
