/*******************************************************
 usbip-server - a platform for USB device prototyping

 Fredrik Andersson
 Copyright 2025, All Rights Reserved.

 This software may be used by anyone for any reason so
 long as the copyright notice in the source files
 remains intact.

 code repository located at:
        http://github.com/freand76/usbip-server
********************************************************/

#ifndef USB_BULK_DESCRIPTORS_H_
#define USB_BULK_DESCRIPTORS_H_

#include <stdint.h>

#include "descriptor_utils.h"

//
// USB Strings
//

static const usb_string_t usb_strings[] = {
    {1, "Virtual UsbIP"},
    {2, "Virtual Bulk Device"},
    {4, "ABC123"},
    {0, NULL},
};

//
// Device Descriptor
//

#define USB_VID 0x00fa
#define USB_PID 0xc001

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
// Configuration ID Descriptor
//

#define USB_BULK_CONFIG_DESC_LENGTH (32)
#define BULK_CMD_PACKET_SZE 8    /* Control Endpoint Packet size */
#define BULK_DATA_PACKET_SZE 512 /* Data Endpoint Packet size */

uint8_t bulk_config_descriptor[USB_BULK_CONFIG_DESC_LENGTH] = {
    0x09,                                /* bLength: Configuration Descriptor size */
    USB_DESCRIPTOR_TYPE_CONFIGURATION,   /* bDescriptorType */
    LOBYTE(USB_BULK_CONFIG_DESC_LENGTH), /* wTotalLength */
    HIBYTE(USB_BULK_CONFIG_DESC_LENGTH), /**/
    0x01,                                /* bNumInterfaces: 1 interfaces */
    0x01,                                /* bConfigurationValue: */
    0x00,                                /* iConfiguration: */
    0xC0,                                /* bmAttributes: */
    0x32,                                /* MaxPower 100 mA */

    /*---------------------------------------------------------------------------*/

    /*Data class interface descriptor*/
    0x09,                          /* bLength: Endpoint Descriptor size */
    USB_DESCRIPTOR_TYPE_INTERFACE, /* bDescriptorType: */
    0x00,                          /* bInterfaceNumber: Number of Interface */
    0x00,                          /* bAlternateSetting: Alternate setting */
    0x02,                          /* bNumEndpoints: Two endpoints used */
    0xFF,                          /* bInterfaceClass: Vendor */
    0x00,                          /* bInterfaceSubClass: */
    0x00,                          /* bInterfaceProtocol: */
    0x00,                          /* iInterface: */

    /*Endpoint OUT Descriptor*/
    0x07,                         /* bLength: Endpoint Descriptor size */
    USB_DESCRIPTOR_TYPE_ENDPOINT, /* bDescriptorType: Endpoint */
    0x01,                         /* bEndpointAddress */
    0x02,                         /* bmAttributes: Bulk */
    LOBYTE(BULK_DATA_PACKET_SZE), /* wMaxPacketSize: */
    HIBYTE(BULK_DATA_PACKET_SZE), /**/
    0x00,                         /* bInterval: ignore for Bulk transfer */

    /*Endpoint IN Descriptor*/
    0x07,                         /* bLength: Endpoint Descriptor size */
    USB_DESCRIPTOR_TYPE_ENDPOINT, /* bDescriptorType: Endpoint */
    0x81,                         /* bEndpointAddress */
    0x02,                         /* bmAttributes: Bulk */
    LOBYTE(BULK_DATA_PACKET_SZE), /* wMaxPacketSize: */
    HIBYTE(BULK_DATA_PACKET_SZE), /**/
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
