/*******************************************************
 usbip-server - a platform for USB device prototyping

 Fredrik Andersson
 Copyright 2016, All Rights Reserved.

 This software may be used by anyone for any reason so
 long as the copyright notice in the source files
 remains intact.

 code repository located at:
        http://github.com/freand76/usbip-server
********************************************************/

#ifndef USB_INTERFACE_H
#define USB_INTERFACE_H

#include <stdint.h>
#include "UsbEndpoint.h"

class UsbInterface {
public:
    UsbInterface(uint8_t bInterfaceNumber,
                 uint8_t bAlternateSetting,
                 uint8_t bInterfaceClass,
                 uint8_t bInterfaceSubClass,
                 uint8_t bInterfaceProtocol,
                 uint8_t iInterface,
                 uint8_t bNumEndpoints,
                 UsbEndpoint** endpointArray);

    virtual int GenerateConfigurationDescriptor(unsigned char* buffer, int offset);
    virtual int InterfaceRequest(unsigned char* setup, unsigned char* data, unsigned char* replyBuffer, int bufLength);

    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
    UsbEndpoint** endpointArray;
};

#endif // USB_INTERFACE_H
