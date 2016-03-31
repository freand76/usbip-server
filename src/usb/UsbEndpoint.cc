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

#include "UsbEndpoint.h"
#include "UsbUtil.h"

using namespace UsbUtil;

UsbEndpoint::UsbEndpoint(uint8_t bEndpointAddress,
			 uint8_t bmAttributes,
			 uint16_t wMaxPacketSize,
			 uint8_t bInterval) {
    this->bEndpointAddress = bEndpointAddress;
    this->bmAttributes  = bmAttributes;
    this->wMaxPacketSize = wMaxPacketSize;
    this->bInterval = bInterval;
}

int UsbEndpoint::GenerateConfigurationDescriptor(unsigned char* buffer, int offset) {
    int pos = offset;

    pos += SetUint(7,                buffer, pos, 1);
    pos += SetUint(5,                buffer, pos, 1);
    pos += SetUint(bEndpointAddress, buffer, pos, 1);
    pos += SetUint(bmAttributes,     buffer, pos, 1);
    pos += SetUint(wMaxPacketSize,   buffer, pos, 2);
    pos += SetUint(bInterval,        buffer, pos, 1);

    return pos - offset;
}
