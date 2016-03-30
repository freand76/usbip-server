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

int UsbEndpoint::GenerateDescriptor(unsigned char* buffer, int offset) {
    buffer[offset + 0] = 7;
    buffer[offset + 1] = 5;
    buffer[offset + 2] = bEndpointAddress;
    buffer[offset + 3] = bmAttributes;
    SetUint(wMaxPacketSize, buffer, offset + 4, 2);
    buffer[offset + 6] = bInterval;
    return 7;
}
