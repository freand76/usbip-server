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

#ifndef TCPIP_SERVER_H_
#define TCPIP_SERVER_H_

#include <stdbool.h>
#include <stddef.h>

bool tcpip_server_open(int port);
void tcpip_server_close(void);
void tcpip_server_stop(void);
void tcpip_server_close_client(void);
bool tcpip_server_wait_for_client(void);
bool tcpip_server_read(void *buffer, size_t read_size);
bool tcpip_server_write(void *buffer, size_t write_size);

#endif
