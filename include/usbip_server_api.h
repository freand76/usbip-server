/*******************************************************
 usbip-server - a platform for USB device prototyping

 Fredrik Andersson
 Copyright 2024-2026, All Rights Reserved.

 This software may be used by anyone for any reason so
 long as the copyright notice in the source files
 remains intact.

 code repository located at:
        http://github.com/freand76/usbip-server
********************************************************/

#ifndef USBIP_SERVER_API_H_
#define USBIP_SERVER_API_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Callback function type for USB EP0 control packets
 *
 * Receives class, vendor and interface requests. Also receives the standard SET_CONFIGURATION
 * request (already handled), so the device knows when the host has configured it.
 */
typedef void(control_data_func_t)(const uint8_t *setup, const uint8_t *data, size_t data_length);

/**
 * @brief Callback function type for USB EP data packets
 */
typedef void(endpoint_data_func_t)(uint8_t ep, const uint8_t *data, size_t data_length);

/**
 * @brief Callback function type for USB reset
 *
 * Called when a host attaches the device and when it disconnects, like a bus reset or a
 * cable being pulled on real hardware. Data transmitted before the reset is discarded.
 */
typedef void(reset_func_t)(void);

/**
 * @brief USB String type used to connect string id with text
 */
typedef struct {
        uint8_t id;
        const char *string;
} usb_string_t;

/**
 * @brief USBIP Device type containing pointers to descriptors, strings and callbacks
 */
typedef struct {
        const uint8_t *device_desc;
        const uint8_t *config_desc;
        const uint8_t *langid_desc;
        const usb_string_t *usb_strings;
        control_data_func_t *control_data_callback;
        endpoint_data_func_t *ep_data_callback;
        reset_func_t *reset_callback; /* Optional */
} usbip_device_t;

/**
 * @brief Transmit EP data from virtual device to host
 *
 * Waits until the host has fetched earlier data on the endpoint. In device callbacks and
 * critical sections it can not wait, and drops the data if the endpoint still has unread data.
 * Data still waiting when the device is reset is dropped as well.
 *
 * @return true if the data was queued for the host
 */
bool usbip_device_transmit(uint8_t ep, const uint8_t *data, size_t data_length);

/**
 * @brief Enter critical section
 *
 * No device callbacks (control, endpoint data or reset) will run while in critical region
 */
void usbip_device_critical_section_enter(void);

/**
 * @brief Exit critical section
 */
void usbip_device_critical_section_exit(void);

/**
 * @brief Start Virtual Device USBIP server
 */
bool usbip_server_start(const usbip_device_t *usb_device);

/**
 * @brief Stop Virtual Device USBIP server
 */
void usbip_server_stop(void);

/**
 * @brief Test if USBIP server has been interrupts (by Ctrl-C)
 */
bool usbip_server_interrupted(void);

#endif
