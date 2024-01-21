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

#ifndef USB_CONFIGURATION_H
#define USB_CONFIGURATION_H

#include <stdint.h>
#include "UsbInterface.h"
#include "UsbEndpoint.h"

class UsbConfiguration {
    public:
        UsbConfiguration(uint8_t bConfigurationValue, uint8_t bNumInterfaces, UsbInterface **interfaceArray, uint8_t iConfiguration = 0,
                         uint8_t bmAttributes = 0xc0, uint8_t bMaxPower = 100);

        int GenerateConfigurationDescriptor(uint8_t *buffer, int offset);
        UsbEndpoint *GetEndpoint(uint8_t endpointAddress);

        uint8_t bNumInterfaces;
        uint8_t bConfigurationValue;
        uint8_t iConfiguration;
        uint8_t bmAttributes;
        uint8_t bMaxPower;
        UsbInterface **interfaceArray;
};

#endif // USB_CONFIGURATION_H
