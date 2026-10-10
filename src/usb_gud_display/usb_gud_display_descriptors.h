/*******************************************************
 usbip-server - a platform for USB device prototyping

 Fredrik Andersson
 Copyright 2026, All Rights Reserved.

 This software may be used by anyone for any reason so
 long as the copyright notice in the source files
 remains intact.

 code repository located at:
        http://github.com/freand76/usbip-server
********************************************************/

#ifndef USB_GUD_DISPLAY_DESCRIPTORS_H_
#define USB_GUD_DISPLAY_DESCRIPTORS_H_

#include <stdint.h>

#include "descriptor_utils.h"
#include "gud_protocol.h"

//
// Display Configuration
//

#define DISPLAY_WIDTH (320)
#define DISPLAY_HEIGHT (240)
#define DISPLAY_REFRESH_HZ (60)

/* Pixel formats offered to the host, in order of preference */
static const uint8_t display_formats[] = {
    GUD_PIXEL_FORMAT_RGB565,
};

/* The host splits updates into transfers of at most this size */
#define DISPLAY_MAX_BUFFER_SIZE (64 * 1024)

/* GUD_COMPRESSION_LZ4 once the device can decompress */
#define DISPLAY_COMPRESSION (0)

//
// USB Strings
//

static const usb_string_t usb_strings[] = {
    {1, "Virtual UsbIP"},
    {2, "Virtual GUD Display"},
    {4, "ABC123"},
    {0, NULL},
};

//
// Device Descriptor
//

/* The Linux gud driver only binds to these ids, this pair is the original GUD id */
#define USB_VID 0x1d50
#define USB_PID 0x614d

static const uint8_t usb_device_descriptor[USB_DEVICE_DESCRIPTOR_LENGTH] = {
    USB_DEVICE_DESCRIPTOR_LENGTH, // bLength
    USB_DESCRIPTOR_TYPE_DEVICE,   // bDescriptorType
    LOBYTE(USB_BCD_VERSION),      // bcdUSB
    HIBYTE(USB_BCD_VERSION),
    0x00,            // bDeviceClass (use class information in the interface descriptors)
    0x00,            // bDeviceSubClass
    0x00,            // bDeviceProtocol
    64,              // bMaxPacketSize
    LOBYTE(USB_VID), // idVendor
    HIBYTE(USB_VID),
    LOBYTE(USB_PID), // idProduct
    HIBYTE(USB_PID),
    LOBYTE(USB_BCD_DEVICE_REL), // bcdDevice - USB device release number
    HIBYTE(USB_BCD_DEVICE_REL),
    1, // Index of manufacturer string
    2, // Index of product string
    4, // Index of serial number string
    1, // bNumConfigurations
};

//
// Configuration Descriptor
//

#define GUD_INTERFACE (0)
#define GUD_EP_DATA_OUT (0x01)
#define GUD_DATA_PACKET_SIZE (512)

#define USB_GUD_CONFIG_DESC_LENGTH (9 + 9 + 7)

static const uint8_t usb_gud_configuration[USB_GUD_CONFIG_DESC_LENGTH] = {
    0x09,                               /* bLength: Configuration Descriptor size */
    USB_DESCRIPTOR_TYPE_CONFIGURATION,  /* bDescriptorType */
    LOBYTE(USB_GUD_CONFIG_DESC_LENGTH), /* wTotalLength */
    HIBYTE(USB_GUD_CONFIG_DESC_LENGTH), /**/
    0x01,                               /* bNumInterfaces */
    0x01,                               /* bConfigurationValue */
    0x00,                               /* iConfiguration */
    0xC0,                               /* bmAttributes: self powered */
    0x32,                               /* MaxPower 100 mA */

    /* Display interface, the gud driver binds to vendor specific interfaces */
    0x09,                          /* bLength */
    USB_DESCRIPTOR_TYPE_INTERFACE, /* bDescriptorType */
    GUD_INTERFACE,                 /* bInterfaceNumber */
    0x00,                          /* bAlternateSetting */
    0x01,                          /* bNumEndpoints */
    0xFF,                          /* bInterfaceClass: Vendor */
    0x00,                          /* bInterfaceSubClass */
    0x00,                          /* bInterfaceProtocol */
    0x00,                          /* iInterface */

    /* Bulk OUT endpoint for pixel data */
    0x07,                         /* bLength */
    USB_DESCRIPTOR_TYPE_ENDPOINT, /* bDescriptorType */
    GUD_EP_DATA_OUT,              /* bEndpointAddress */
    0x02,                         /* bmAttributes: Bulk */
    LOBYTE(GUD_DATA_PACKET_SIZE), /* wMaxPacketSize */
    HIBYTE(GUD_DATA_PACKET_SIZE), /**/
    0x00,                         /* bInterval */
};

//
// Language ID Descriptor
//

#define USB_LANGID_STRING 0x409 /* 1033 US.S English */
#define USB_LANGID_DESC_LENGTH (4)

static const uint8_t langid_desc[USB_LANGID_DESC_LENGTH] = {
    USB_LANGID_DESC_LENGTH,
    USB_DESCRIPTOR_TYPE_STRING,
    LOBYTE(USB_LANGID_STRING),
    HIBYTE(USB_LANGID_STRING),
};

#endif
