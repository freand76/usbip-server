/*******************************************************
 usbip-server - a platform for USB device prototyping

 Fredrik Andersson
 Copyright 2024-2026

 SPDX-License-Identifier: MIT
 See the LICENSE file in the repository.

 code repository located at:
        https://github.com/freand76/usbip-server
********************************************************/

/**
 * @file tcpip_server.h
 * @brief TCP connection of the server, one listening socket and at most one client
 */

#ifndef TCPIP_SERVER_H_
#define TCPIP_SERVER_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Listen for clients
 *
 * Also ignores SIGPIPE, so a client that disappears does not end the program.
 *
 * @param address IPv4 address to listen on, for example "127.0.0.1" or "0.0.0.0"
 * @param port    TCP port
 * @return false if the address is invalid or the socket can not be bound
 */
bool tcpip_server_open(const char *address, int port);

/**
 * @brief Close the listening socket
 */
void tcpip_server_close(void);

/**
 * @brief Make waiting reads, writes and waits return
 *
 * Only sets a flag, so it may be called from a signal handler.
 */
void tcpip_server_stop(void);

/**
 * @brief Close the connection to the client
 */
void tcpip_server_close_client(void);

/**
 * @brief Wait at most 500 ms for a client and accept it
 *
 * Sets up the connection: a receive timeout so reads can notice tcpip_server_stop(), no
 * Nagle delay, and TCP keepalive to detect clients that disappear.
 *
 * @return true if a client is connected
 */
bool tcpip_server_wait_for_client(void);

/**
 * @brief Wait until the client has data, wake_fd is signalled or the timeout expires
 *
 * @param wake_fd    A second file descriptor to wait for, see usbip_device_get_event_fd()
 * @param timeout_us Longest time to wait
 * @return true if client data is available, or the connection has an error that the
 *         next read reports
 */
bool tcpip_server_wait_for_data(int wake_fd, uint64_t timeout_us);

/**
 * @brief Read exactly read_size bytes from the client
 *
 * Keeps waiting while the client is silent.
 *
 * @return false if the client disconnected or the server was stopped
 */
bool tcpip_server_read(void *buffer, size_t read_size);

/**
 * @brief Write write_size bytes to the client
 *
 * @return false if the client disconnected or the server was stopped
 */
bool tcpip_server_write(void *buffer, size_t write_size);

#endif
