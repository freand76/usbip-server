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
 * @file usbip_server_api.h
 * @brief Public API for writing a virtual USB device
 *
 * A device is described by a usbip_device_t: its descriptors, strings and callbacks. Pass it
 * to usbip_server_start(), which serves it over USB/IP on TCP port 3240 until
 * usbip_server_stop(). A host attaches it with "usbip attach -r <host> -b 1-1".
 *
 * Threads: the server runs in its own thread, and all device callbacks are called from that
 * thread, never at the same time. Use usbip_device_critical_section_enter() and
 * usbip_device_critical_section_exit() to share data between callbacks and the rest of the
 * program.
 *
 * The device is always reported as high speed, so descriptors must follow the high speed
 * rules: bMaxPacketSize0 64, bulk endpoints with 512 byte packets, and interrupt and
 * isochronous bInterval as an exponent, 2^(bInterval - 1) * 125 us.
 */

#ifndef USBIP_SERVER_API_H_
#define USBIP_SERVER_API_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Callback function type for USB EP0 control packets
 *
 * Receives class, vendor and interface requests. Also receives the standard SET_CONFIGURATION
 * request (already handled), so the device knows when the host has configured it. The other
 * standard requests are answered by the library.
 *
 * Answer an IN request with usbip_device_transmit(0, ...) before returning, an IN request
 * that gets no answer is stalled. An OUT request is always accepted.
 *
 * @param setup       The 8 byte setup packet: bmRequestType, bRequest, wValue, wIndex and
 *                    wLength, multi byte fields little endian
 * @param data        The data stage of an OUT request, meaningless for an IN request
 * @param data_length Length of the data stage, wLength
 */
typedef void(control_data_func_t)(const uint8_t *setup, const uint8_t *data, size_t data_length);

/**
 * @brief Callback function type for USB EP data packets
 *
 * Called with the data of each transfer the host sends to a bulk or interrupt OUT endpoint,
 * and with each packet sent to an isochronous OUT endpoint.
 *
 * @param ep          Endpoint number, without the direction bit
 * @param data        The received data
 * @param data_length Number of bytes received
 */
typedef void(endpoint_data_func_t)(uint8_t ep, const uint8_t *data, size_t data_length);

/**
 * @brief Callback function type for isochronous IN endpoints
 *
 * Called once per service interval of the endpoint, as the host reads it. Write the data for
 * this interval, at most max_length bytes, to buffer and return how many bytes were written.
 *
 * @param ep         Endpoint number, without the direction bit
 * @param buffer     Where to write the data for this service interval
 * @param max_length Most bytes the host accepts for this interval
 * @return Number of bytes written, 0 for an empty packet
 */
typedef size_t(iso_in_data_func_t)(uint8_t ep, uint8_t *buffer, size_t max_length);

/**
 * @brief Callback function type for USB reset
 *
 * Called when a host attaches the device and when it disconnects, like a bus reset or a
 * cable being pulled on real hardware. Data transmitted before the reset is discarded.
 */
typedef void(reset_func_t)(void);

/**
 * @brief USB String type used to connect string id with text
 *
 * An array of these lists the device's strings, ended by an entry with id 0. String index 0
 * is the language id descriptor and is not part of the array. Strings are ASCII and at most
 * 126 characters long.
 */
typedef struct {
        uint8_t id;         /**< String index used in the descriptors */
        const char *string; /**< The text */
} usb_string_t;

/**
 * @brief USBIP Device type containing pointers to descriptors, strings and callbacks
 */
typedef struct {
        /** Device descriptor, 18 bytes */
        const uint8_t *device_desc;
        /** Configuration descriptor with all its interfaces and endpoints, wTotalLength bytes */
        const uint8_t *config_desc;
        /** String descriptor 0, the list of supported language ids */
        const uint8_t *langid_desc;
        /** The device's strings, ended by an entry with id 0 */
        const usb_string_t *usb_strings;
        /** Called for control requests the library does not answer itself */
        control_data_func_t *control_data_callback;
        /** Called with data received on OUT endpoints, may be NULL */
        endpoint_data_func_t *ep_data_callback;
        reset_func_t *reset_callback;        /**< Called on attach and detach, optional */
        iso_in_data_func_t *iso_in_callback; /**< Fills isochronous IN packets, optional */
} usbip_device_t;

/**
 * @brief Transmit EP data from virtual device to host
 *
 * Waits until the host has fetched earlier data on the endpoint. In device callbacks and
 * critical sections it can not wait, and drops the data if the endpoint still has unread data.
 * Data still waiting when the device is reset is dropped as well.
 *
 * On endpoint 0 it answers the current control IN request, data_length 0 (data may be NULL)
 * is a valid empty answer. A control IN request that gets no answer is stalled.
 *
 * Data that does not fit the host's transfer is delivered in the following transfers.
 * Interrupt endpoints deliver at most one transfer per polling interval.
 *
 * @param ep          Endpoint number, the direction bit is ignored
 * @param data        Data to send
 * @param data_length Number of bytes, at most 512, longer data is cut
 * @return true if the data was queued for the host
 */
bool usbip_device_transmit(uint8_t ep, const uint8_t *data, size_t data_length);

/**
 * @brief Enter critical section
 *
 * No device callbacks (control, endpoint data or reset) will run while in critical region
 *
 * Keep critical sections short, the host waits while a callback can not run. Critical
 * sections can not be nested.
 */
void usbip_device_critical_section_enter(void);

/**
 * @brief Exit critical section
 */
void usbip_device_critical_section_exit(void);

/**
 * @brief Start Virtual Device USBIP server
 *
 * Starts the server thread and listens on TCP port 3240 of 127.0.0.1, or of the IPv4 address
 * in the environment variable USBIP_BIND_ADDRESS. Also installs a SIGINT handler: Ctrl-C
 * three times interrupts the server, five more times forces an exit.
 *
 * @param usb_device The device to serve, must stay valid until usbip_server_stop()
 * @return true if the server is running
 */
bool usbip_server_start(const usbip_device_t *usb_device);

/**
 * @brief Stop Virtual Device USBIP server
 *
 * Disconnects the client, stops the server thread and waits for it to end.
 */
void usbip_server_stop(void);

/**
 * @brief Test if USBIP server has been interrupted (by Ctrl-C)
 *
 * @return true once Ctrl-C has been pressed three times, the main loop should then call
 *         usbip_server_stop() and exit
 */
bool usbip_server_interrupted(void);

#endif
