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

#ifndef USBIP_DEVICE_H_
#define USBIP_DEVICE_H_

#include <stddef.h>
#include <stdint.h>

#include "usbip_server_api.h"

void usbip_device_stop(void);

uint8_t *usbip_device_get_ep_buffer(uint8_t ep, size_t *buffer_size);

void usbip_device_release_buffer(uint8_t ep);

void usbip_device_control(const usbip_device_t *usb_device, uint8_t *setup, uint8_t *buffer,
                          size_t length);

#endif
