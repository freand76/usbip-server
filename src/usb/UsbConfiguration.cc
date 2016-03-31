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

#include "UsbConfiguration.h"
#include "UsbUtil.h"

using namespace UsbUtil;

UsbConfiguration::UsbConfiguration(uint8_t bConfigurationValue,
				   uint8_t iConfiguration,
				   uint8_t bmAttributes,
				   uint8_t bMaxPower,
				   uint8_t bNumInterfaces,
				   UsbInterface** interfaceArray) {
    this->bNumInterfaces = bNumInterfaces;
    this->bConfigurationValue = bConfigurationValue;
    this->iConfiguration = iConfiguration;
    this->bmAttributes = bmAttributes;
    this->bMaxPower = bMaxPower;
    this->interfaceArray = interfaceArray;
}

int UsbConfiguration::GenerateConfigurationDescriptor(unsigned char* buffer, int offset) {
    int pos = offset;

    pos += SetUint(9,                   buffer, pos, 1);
    pos += SetUint(2,                   buffer, pos, 1);
    pos += SetUint(0,                   buffer, pos, 2); /* total size will be filed later */
    pos += SetUint(bNumInterfaces,      buffer, pos, 1);
    pos += SetUint(bConfigurationValue, buffer, pos, 1);
    pos += SetUint(iConfiguration,      buffer, pos, 1);
    pos += SetUint(bmAttributes,        buffer, pos, 1);
    pos += SetUint(bMaxPower,           buffer, pos, 1);

    for (int idx = 0; idx < bNumInterfaces; idx++) {
	pos += interfaceArray[idx]->GenerateConfigurationDescriptor(buffer, offset + pos);
    }

    /* Set total size */
    SetUint(pos, buffer, 2, 2);
    return pos-offset;
}
