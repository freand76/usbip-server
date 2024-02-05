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

#ifndef USBIP_PROTOCOL_H_
#define USBIP_PROTOCOL_H_

#include <stdbool.h>

#include "usbip_device.h"

bool usbip_protocol_process_cmd(const usbip_device_t *usb_device);

bool usbip_protocol_process_imported_device(const usbip_device_t *usb_device);

bool usbip_protocol_is_device_imported(void);

#endif
