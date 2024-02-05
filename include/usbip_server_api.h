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

typedef void(control_data_func_t)(const uint8_t *setup, const uint8_t *data, size_t data_length);

typedef void(endpoint_data_func_t)(uint8_t ep, const uint8_t *data, size_t data_length);

typedef struct {
        uint8_t id;
        const char *string;
} usb_string_t;

typedef struct {
        const uint8_t *device_desc;
        const uint8_t *config_desc;
        const uint8_t *langid_desc;
        const usb_string_t *usb_strings;
        control_data_func_t *control_data_callback;
        endpoint_data_func_t *ep_data_callback;
} usbip_device_t;

void usbip_device_transmit(uint8_t ep, const uint8_t *data, size_t data_length);

bool usbip_server_start(const usbip_device_t *usb_device);

bool usbip_server_interrupted(void);

void usbip_server_stop(void);

#endif
