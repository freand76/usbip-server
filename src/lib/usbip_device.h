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

#ifndef USBIP_DEVICE_H_
#define USBIP_DEVICE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "usbip_server_api.h"

bool usbip_device_init(void);

/* Must be called from the thread that serves the client */
void usbip_device_mark_server_thread(void);

/* Return to the state of a freshly attached device, like a USB bus reset */
void usbip_device_reset(const usbip_device_t *usb_device);

void usbip_device_stop(void);

void usbip_device_receive(const usbip_device_t *usb_device, uint8_t ep, uint8_t *data,
                          size_t data_length);

/* File descriptor that becomes readable when IN endpoint data is available */
int usbip_device_get_event_fd(void);

void usbip_device_clear_event(void);

size_t usbip_device_fetch_ep_data(uint8_t ep, uint8_t *buffer, size_t max_length);

/* Returns false when the request is not supported and must be stalled */
bool usbip_device_control(const usbip_device_t *usb_device, uint8_t *setup, uint8_t *buffer,
                          size_t length);

#endif
