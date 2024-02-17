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

#ifndef USB_MOUSE_DESCRIPTORS_H_
#define USB_MOUSE_DESCRIPTORS_H_

#include <stdint.h>

#include "descriptor_utils.h"

//
// USB Strings
//

static const usb_string_t usb_strings[] = {
    {1, "Virtual UsbIP"},
    {2, "Virtual HID Mouse"},
    {4, "ABC123"},
    {0, NULL},
};

//
// Device Descriptor
//

#define USB_VID 0xfafa
#define USB_PID 0x0002

static const uint8_t usb_mouse_device[USB_DEVICE_DESCRIPTOR_LENGTH] = {
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

#define USB_MOUSE_CONFIG_DESC_LENGTH (34)
#define HID_MOUSE_REPORT_DESC_LENGTH (50)

#define USB_MOUSE_HID_EP_DATA_IN (0x81)
#define USB_MOUSE_HID_POLL_INTERVAL_MS (10)

#define USB_HID_BCD 0x111

static const uint8_t usb_mouse_configuration[USB_MOUSE_CONFIG_DESC_LENGTH] = {
    0x09,                                       /* bLength: Configuration Descriptor size */
    USB_DESCRIPTOR_TYPE_CONFIGURATION,          /* bDescriptorType: Configuration */
    LOBYTE(USB_MOUSE_CONFIG_DESC_LENGTH),       /* wTotalLength: Bytes returned */
    HIBYTE(USB_MOUSE_CONFIG_DESC_LENGTH), 0x01, /*bNumInterfaces: 1 interface*/
    0x01,                                       /*bConfigurationValue: Configuration value*/
    0x00, /*iConfiguration: Index of string descriptor describing the configuration*/
    0xC0, /*bmAttributes: bus powered and Support Remote Wake-up */
    0x32, /*MaxPower 100 mA: this current is used for detecting Vbus*/

    /************** Descriptor of Joystick Mouse interface ****************/
    0x09,                          /*bLength: Interface Descriptor size*/
    USB_DESCRIPTOR_TYPE_INTERFACE, /*bDescriptorType: Interface descriptor type*/
    0x00,                          /*bInterfaceNumber: Number of Interface*/
    0x00,                          /*bAlternateSetting: Alternate setting*/
    0x01,                          /*bNumEndpoints*/
    0x03,                          /*bInterfaceClass: HID*/
    0x01,                          /*bInterfaceSubClass : 1=BOOT, 0=no boot*/
    0x02,                          /*nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse*/
    0,                             /*iInterface: Index of string descriptor*/
    /******************** Descriptor of Joystick Mouse HID ********************/
    0x09,                      /*bLength: HID Descriptor size*/
    USB_DESCRIPTOR_TYPE_HID,   /*bDescriptorType: HID*/
    LOBYTE(USB_HID_BCD),       /*bcdHID: HID Class Spec release number*/
    HIBYTE(USB_HID_BCD), 0x00, /*bCountryCode: Hardware target country*/
    0x01,                      /*bNumDescriptors: Number of HID class descriptors to follow*/
    0x22,                      /*bDescriptorType*/
    LOBYTE(HID_MOUSE_REPORT_DESC_LENGTH), /*wItemLength: Total length of Report descriptor*/
    HIBYTE(HID_MOUSE_REPORT_DESC_LENGTH),
    /******************** Descriptor of Mouse endpoint ********************/
    0x07,                         /*bLength: Endpoint Descriptor size*/
    USB_DESCRIPTOR_TYPE_ENDPOINT, /*bDescriptorType:*/

    USB_MOUSE_HID_EP_DATA_IN,             /*bEndpointAddress: Endpoint Address (IN)*/
    0x03,                                 /*bmAttributes: Interrupt endpoint*/
    0x04,                                 /*wMaxPacketSize: 4 Byte max */
    0x00, USB_MOUSE_HID_POLL_INTERVAL_MS, /*bInterval: Polling Interval (10 ms)*/
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

//
// HID Report Descriptor
//

static uint8_t usb_mouse_hid_report[] = {
    0x05, 0x01, // USAGE_PAGE (Generic Desktop)
    0x09, 0x02, // USAGE (Mouse)
    0xa1, 0x01, // COLLECTION (Application)
    0x09, 0x01, //   USAGE (Pointer)
    0xa1, 0x00, //   COLLECTION (Physical)
    0x05, 0x09, //     USAGE_PAGE (Button)
    0x19, 0x01, //     USAGE_MINIMUM (Button 1)
    0x29, 0x03, //     USAGE_MAXIMUM (Button 3)
    0x15, 0x00, //     LOGICAL_MINIMUM (0)
    0x25, 0x01, //     LOGICAL_MAXIMUM (1)
    0x75, 0x01, //     REPORT_SIZE (1)
    0x95, 0x03, //     REPORT_COUNT (3)
    0x81, 0x02, //     INPUT (Data,Var,Abs)
    0x95, 0x01, //     REPORT_COUNT (1)
    0x75, 0x05, //     REPORT_SIZE (5)
    0x81, 0x01, //     INPUT (Cnst,Var,Abs)
    0x05, 0x01, //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30, //     USAGE (X)
    0x09, 0x31, //     USAGE (Y)
    0x15, 0x81, //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f, //     LOGICAL_MAXIMUM (127)
    0x75, 0x08, //     REPORT_SIZE (8)
    0x95, 0x03, //     REPORT_COUNT (2)
    0x81, 0x06, //     INPUT (Data,Var,Rel)
    0xc0,       //   END_COLLECTION
    0xc0,       //   END_COLLECTION
};

#endif
