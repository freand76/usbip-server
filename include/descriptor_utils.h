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

#ifndef DESCRIPTOR_UTILS_H_
#define DESCRIPTOR_UTILS_H_

#define LOBYTE(x) (x & 0xff)
#define HIBYTE(x) ((x >> 8) & 0xff)

#define USB_DEVICE_DESCRIPTOR_LENGTH (18)
#define USB_BCD_DEVICE_REL (0x0200)
#define USB_BCD_VERSION (0x0200)

#define USB_DESCRIPTOR_TYPE_DEVICE (0x01)
#define USB_DESCRIPTOR_TYPE_CONFIGURATION (0x02)
#define USB_DESCRIPTOR_TYPE_STRING (0x03)
#define USB_DESCRIPTOR_TYPE_INTERFACE (0x04)
#define USB_DESCRIPTOR_TYPE_ENDPOINT (0x05)

#define USB_DESCRIPTOR_TYPE_HID (0x21)
#define USB_DESCRIPTOR_TYPE_REPORT (0x22)

#define USB_REQUEST_GET_STATUS (0)
#define USB_REQUEST_GET_DESCRIPTOR (6)

#endif
