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

#ifndef USB_ENDPOINT_H
#define USB_ENDPOINT_H

#include <stdint.h>

class UsbEndpoint {
    public:
        UsbEndpoint(uint8_t bEndpointAddress, uint8_t bmAttributes, uint16_t wMaxPacketSize, uint8_t bInterval);
        int GenerateConfigurationDescriptor(uint8_t *buffer, int offset);
        virtual int Data(uint8_t *dataIn, uint8_t *dataOut, int transferLength) = 0;

        uint8_t bEndpointAddress;
        uint8_t bmAttributes;
        uint16_t wMaxPacketSize;
        uint8_t bInterval;
};

#endif // USB_ENDPOINT_H
