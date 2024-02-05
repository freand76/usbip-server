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

#ifndef USB_CDC_DESCRIPTORS_H_
#define USB_CDC_DESCRIPTORS_H_

#include <stdint.h>

#include "descriptor_utils.h"

//
// USB Strings
//

static const usb_string_t usb_strings[] = {
    {1, "Virtual UsbIP"},
    {2, "Virtual CDC Device"},
    {4, "ABC123"},
    {0, NULL},
};

//
// Device Descriptor
//

#define USB_LEN_DEV_DESC (18)
#define USB_VID 0xfafa
#define USB_PID 0x0001
#define USB_DEV_RELEASE 0x0101

static const uint8_t usb_device_descriptor[USB_LEN_DEV_DESC] = {
    USB_LEN_DEV_DESC, // bLength
    0x01,             // bDescriptorType
    0x00,             // bcdUSB
    0x02,
    0x00,            // bDeviceClass (use class information in the interface descriptors)
    0x00,            // bDeviceSubClass
    0x00,            // bDeviceProtocol
    64,              // bMaxPacketSize
    LOBYTE(USB_VID), // idVendor
    HIBYTE(USB_VID),
    LOBYTE(USB_PID), // idProduct
    HIBYTE(USB_PID),
    LOBYTE(USB_DEV_RELEASE), // bcdDevice - USB device release number
    HIBYTE(USB_DEV_RELEASE),
    1, // Index of manufacturer string
    2, // Index of product string
    4, // Index of serial number string
    1, // bNumConfigurations
};

//
// Configuration ID Descriptor
//

#define USB_CDC_CONFIG_DESC_SIZ (67)
#define CDC_CMD_PACKET_SZE 8    /* Control Endpoint Packet size */
#define CDC_DATA_PACKET_SZE 512 /* Data Endpoint Packet size */

uint8_t cdc_config_descriptor[USB_CDC_CONFIG_DESC_SIZ] = {
    0x09,                                  /* bLength: Configuration Descriptor size */
    0x02,                                  /* bDescriptorType */
    LOBYTE(USB_CDC_CONFIG_DESC_SIZ),       /* wTotalLength */
    HIBYTE(USB_CDC_CONFIG_DESC_SIZ), 0x02, /* bNumInterfaces: 2 interfaces */
    0x01,                                  /* bConfigurationValue: */
    0x04,                                  /* iConfiguration: */
    0xC0,                                  /* bmAttributes: */
    0x32,                                  /* MaxPower 100 mA */

    /*Interface Descriptor */
    0x09, /* bLength: Interface Descriptor size */
    0x04, /* bDescriptorType: Interface */
    /* Interface descriptor type */
    0x00, /* bInterfaceNumber: Number of Interface */
    0x00, /* bAlternateSetting: Alternate setting */
    0x01, /* bNumEndpoints: One endpoints used */
    0x02, /* bInterfaceClass: Communication Interface Class */
    0x02, /* bInterfaceSubClass: Abstract Control Model */
    0x01, /* bInterfaceProtocol: Common AT commands */
    0x00, /* iInterface: */

    /*Header Functional Descriptor*/
    0x05, /* bLength: Endpoint Descriptor size */
    0x24, /* bDescriptorType: CS_INTERFACE */
    0x00, /* bDescriptorSubtype: Header Func Desc */
    0x10, /* bcdCDC: spec release number */
    0x01,

    /*Call Management Functional Descriptor*/
    0x05, /* bFunctionLength */
    0x24, /* bDescriptorType: CS_INTERFACE */
    0x01, /* bDescriptorSubtype: Call Management Func Desc */
    0x00, /* bmCapabilities: D0+D1 */
    0x01, /* bDataInterface: 1 */

    /*ACM Functional Descriptor*/
    0x04, /* bFunctionLength */
    0x24, /* bDescriptorType: CS_INTERFACE */
    0x02, /* bDescriptorSubtype: Abstract Control Management desc */
    0x02, /* bmCapabilities */

    /*Union Functional Descriptor*/
    0x05, /* bFunctionLength */
    0x24, /* bDescriptorType: CS_INTERFACE */
    0x06, /* bDescriptorSubtype: Union func desc */
    0x00, /* bMasterInterface: Communication class interface */
    0x01, /* bSlaveInterface0: Data Class Interface */

    /*Endpoint 2 Descriptor*/
    0x07,                             /* bLength: Endpoint Descriptor size */
    0x05,                             /* bDescriptorType: Endpoint */
    0x82,                             /* bEndpointAddress */
    0x03,                             /* bmAttributes: Interrupt */
    LOBYTE(CDC_CMD_PACKET_SZE),       /* wMaxPacketSize: */
    HIBYTE(CDC_CMD_PACKET_SZE), 0x10, /* bInterval: */

    /*---------------------------------------------------------------------------*/

    /*Data class interface descriptor*/
    0x09, /* bLength: Endpoint Descriptor size */
    0x04, /* bDescriptorType: */
    0x01, /* bInterfaceNumber: Number of Interface */
    0x00, /* bAlternateSetting: Alternate setting */
    0x02, /* bNumEndpoints: Two endpoints used */
    0x0A, /* bInterfaceClass: CDC */
    0x00, /* bInterfaceSubClass: */
    0x00, /* bInterfaceProtocol: */
    0x00, /* iInterface: */

    /*Endpoint OUT Descriptor*/
    0x07,                              /* bLength: Endpoint Descriptor size */
    0x05,                              /* bDescriptorType: Endpoint */
    0x01,                              /* bEndpointAddress */
    0x02,                              /* bmAttributes: Bulk */
    LOBYTE(CDC_DATA_PACKET_SZE),       /* wMaxPacketSize: */
    HIBYTE(CDC_DATA_PACKET_SZE), 0x00, /* bInterval: ignore for Bulk transfer */

    /*Endpoint IN Descriptor*/
    0x07,                             /* bLength: Endpoint Descriptor size */
    0x05,                             /* bDescriptorType: Endpoint */
    0x81,                             /* bEndpointAddress */
    0x02,                             /* bmAttributes: Bulk */
    LOBYTE(CDC_DATA_PACKET_SZE),      /* wMaxPacketSize: */
    HIBYTE(CDC_DATA_PACKET_SZE), 0x00 /* bInterval */
};

//
// Language ID Descriptor
//

#define USB_LANGID_STRING 0x409 /* 1033 US.S English */
#define USB_LEN_LANGID_STR_DESC (4)

static const uint8_t langid_desc[USB_LEN_LANGID_STR_DESC] = {
    0x04,
    0x03,
    LOBYTE(USB_LANGID_STRING),
    HIBYTE(USB_LANGID_STRING),
};

#endif
