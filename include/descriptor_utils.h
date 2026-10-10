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

/**
 * @file descriptor_utils.h
 * @brief Helpers and constants for writing USB descriptors and handling control requests
 *
 * Values are from chapter 9 of the USB 2.0 specification, the HID values from the HID 1.11
 * class definition.
 */

#ifndef DESCRIPTOR_UTILS_H_
#define DESCRIPTOR_UTILS_H_

/** @brief Low byte of a 16 bit value, descriptors store multi byte fields little endian */
#define LOBYTE(x) (x & 0xff)
/** @brief High byte of a 16 bit value */
#define HIBYTE(x) ((x >> 8) & 0xff)

/** @brief Length of a device descriptor */
#define USB_DEVICE_DESCRIPTOR_LENGTH (18)
/** @brief bcdDevice of the examples, device release 2.00 */
#define USB_BCD_DEVICE_REL (0x0200)
/** @brief bcdUSB, USB 2.0 */
#define USB_BCD_VERSION (0x0200)

/** @name Descriptor types, bDescriptorType
 * @{ */
#define USB_DESCRIPTOR_TYPE_DEVICE (0x01)
#define USB_DESCRIPTOR_TYPE_CONFIGURATION (0x02)
#define USB_DESCRIPTOR_TYPE_STRING (0x03)
#define USB_DESCRIPTOR_TYPE_INTERFACE (0x04)
#define USB_DESCRIPTOR_TYPE_ENDPOINT (0x05)
#define USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER (0x06)
#define USB_DESCRIPTOR_TYPE_OTHER_SPEED_CONFIGURATION (0x07)
/** @} */

/** @name HID class descriptor types
 * @{ */
#define USB_DESCRIPTOR_TYPE_HID (0x21)
#define USB_DESCRIPTOR_TYPE_REPORT (0x22)
/** @} */

/** @name Standard requests, bRequest of the setup packet
 * @{ */
#define USB_REQUEST_GET_STATUS (0)
#define USB_REQUEST_CLEAR_FEATURE (1)
#define USB_REQUEST_SET_FEATURE (3)
#define USB_REQUEST_GET_DESCRIPTOR (6)
#define USB_REQUEST_GET_CONFIGURATION (8)
#define USB_REQUEST_SET_CONFIGURATION (9)
#define USB_REQUEST_GET_INTERFACE (10)
#define USB_REQUEST_SET_INTERFACE (11)
/** @} */

#endif
