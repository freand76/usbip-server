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
#include "UsbUtil.h"

using namespace UsbUtil;

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

int UsbInterface::GenerateConfigurationDescriptor(unsigned char* buffer, int offset) {
    int pos = offset;

    pos += SetUint(9,                  buffer, pos, 1);
    pos += SetUint(4,                  buffer, pos, 1);
    pos += SetUint(bInterfaceNumber,   buffer, pos, 1);
    pos += SetUint(bAlternateSetting,  buffer, pos, 1);
    pos += SetUint(bNumEndpoints,      buffer, pos, 1);
    pos += SetUint(bInterfaceClass,    buffer, pos, 1);
    pos += SetUint(bInterfaceSubClass, buffer, pos, 1);
    pos += SetUint(bInterfaceProtocol, buffer, pos, 1);
    pos += SetUint(iInterface,         buffer, pos, 1);

    for (int idx = 0; idx < bNumEndpoints; idx++) {
	pos += endpointArray[idx]->GenerateConfigurationDescriptor(buffer, pos);
    }
    return pos-offset;
}

int UsbInterface::InterfaceRequest(unsigned char* setup, unsigned char* data, unsigned char* replyBuffer, int bufLength) {
    (void)setup;
    (void)data;
    (void)replyBuffer;
    (void)bufLength;

    return 0;
}
