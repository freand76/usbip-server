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

#ifndef USBIP_SERVER_API_H_
#define USBIP_SERVER_API_H_

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Callback function type for USB EP0 control packets
 */
typedef void(control_data_func_t)(const uint8_t *setup, const uint8_t *data, size_t data_length);

/**
 * @brief Callback function type for USB EP data packets
 */
typedef void(endpoint_data_func_t)(uint8_t ep, const uint8_t *data, size_t data_length);

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
} usbip_device_t;

/**
 * @brief Transmit EP data from virtual device to host
 */
void usbip_device_transmit(uint8_t ep, const uint8_t *data, size_t data_length);

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
