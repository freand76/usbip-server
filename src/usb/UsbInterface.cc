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
#include "UsbInterface.h"

UsbInterface::UsbInterface(uint8_t bInterfaceNumber,
			   uint8_t bAlternateSetting,
			   uint8_t bInterfaceClass,
			   uint8_t bInterfaceSubClass,
			   uint8_t bInterfaceProtocol,
			   uint8_t iInterface,
			   uint8_t bNumEndpoints,
			   UsbEndpoint** endpointArray) {
    this->bInterfaceNumber = bInterfaceNumber;
    this->bAlternateSetting = bAlternateSetting;
    this->bNumEndpoints = bNumEndpoints;
    this->bInterfaceClass = bInterfaceClass;
    this->bInterfaceSubClass = bInterfaceSubClass;
    this->bInterfaceProtocol = bInterfaceProtocol;
    this->iInterface = iInterface;
    this->endpointArray = endpointArray;
}

int UsbInterface::GenerateDescriptor(unsigned char* buffer, int offset) {
    buffer[offset + 0] = 9;
    buffer[offset + 1] = 4;
    buffer[offset + 2] = bInterfaceNumber;
    buffer[offset + 3] = bAlternateSetting;
    buffer[offset + 4] = bNumEndpoints;
    buffer[offset + 5] = bInterfaceClass;
    buffer[offset + 6] = bInterfaceSubClass;
    buffer[offset + 7] = bInterfaceProtocol;
    buffer[offset + 8] = iInterface;

    int pos = 9;
    for (int idx = 0; idx < bNumEndpoints; idx++) {
	pos += endpointArray[idx]->GenerateDescriptor(buffer, offset + pos);
    }
    return pos;
}
