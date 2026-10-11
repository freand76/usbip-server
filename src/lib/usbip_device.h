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
 * @file usbip_device.h
 * @brief The device side of the server: endpoint buffers, device callbacks and the standard
 *        control requests
 *
 * The functions here are called from the server thread, except usbip_device_init(), which
 * usbip_server_start() calls before the thread starts. The public functions in
 * usbip_server_api.h may be called by the device from any thread.
 */

#ifndef USBIP_DEVICE_H_
#define USBIP_DEVICE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "usbip_server_api.h"

/**
 * @brief Set up the device side, before the server thread starts
 *
 * @return false if the event file descriptor can not be created
 */
bool usbip_device_init(void);

/**
 * @brief Mark the calling thread as the server thread
 *
 * Must be called from the thread that serves the client. usbip_device_transmit() never waits
 * on that thread, since only it can fetch the data.
 */
void usbip_device_mark_server_thread(void);

/**
 * @brief Return to the state of a freshly attached device, like a USB bus reset
 *
 * Discards unread endpoint data, also data a waiting usbip_device_transmit() is about to
 * queue, sets the configuration back to 0 and calls the device's reset callback.
 */
void usbip_device_reset(const usbip_device_t *usb_device);

/**
 * @brief Stop accepting endpoint data, and release transmits waiting for the host
 */
void usbip_device_stop(void);

/**
 * @brief Pass data received on an OUT endpoint to the device's endpoint data callback
 *
 * The callback runs inside the device's critical section. The caller checks that the
 * callback is set.
 */
void usbip_device_receive(const usbip_device_t *usb_device, uint8_t ep, uint8_t *data,
                          size_t data_length);

/**
 * @brief Data for one service interval of an isochronous IN endpoint
 *
 * Calls the device's iso IN callback inside the device's critical section.
 *
 * @return Bytes written to buffer, at most max_length, 0 without a callback
 */
size_t usbip_device_iso_in(const usbip_device_t *usb_device, uint8_t ep, uint8_t *buffer,
                           size_t max_length);

/**
 * @brief File descriptor that becomes readable when IN endpoint data is available
 *
 * Lets the server thread wait for the client and for the device at the same time.
 */
int usbip_device_get_event_fd(void);

/**
 * @brief Clear the event, before checking for IN data, so new data wakes the next wait
 */
void usbip_device_clear_event(void);

/**
 * @brief Take up to max_length bytes of data the device queued for an IN endpoint
 *
 * Data that does not fit stays queued for the next transfer, except on endpoint 0 where a
 * control transfer ends at wLength. Releases a waiting usbip_device_transmit() once the
 * endpoint is empty.
 *
 * @return Number of bytes copied to buffer, 0 if the endpoint has no data
 */
size_t usbip_device_fetch_ep_data(uint8_t ep, uint8_t *buffer, size_t max_length);

/**
 * @brief Reply to the current control request
 *
 * @param length Set to the number of bytes copied to buffer
 * @return false when the device did not answer the request, which must then be stalled
 */
bool usbip_device_fetch_control_reply(uint8_t *buffer, size_t max_length, size_t *length);

/**
 * @brief Handle a control request
 *
 * Answers the standard requests the library supports and passes the others to the device's
 * control callback. Any answer is fetched with usbip_device_fetch_control_reply().
 *
 * @param setup  The 8 byte setup packet
 * @param buffer The data stage of an OUT request
 * @param length wLength
 * @return false when the request is not supported and must be stalled
 */
bool usbip_device_control(const usbip_device_t *usb_device, uint8_t *setup, uint8_t *buffer,
                          size_t length);

#endif
