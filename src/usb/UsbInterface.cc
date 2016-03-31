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

#include "UsbDevice.h"
#include "UsbEndpoint.h"
#include "UsbInterface.h"
#include "UsbUtil.h"

using namespace UsbUtil;

#include "Verbose.h"
using namespace Verbose;

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

int UsbInterface::GenerateConfigurationDescriptor(uint8_t* buffer, int offset) {
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

int UsbInterface::InterfaceRequest(uint8_t* setup, uint8_t* data, uint8_t* replyBuffer, int bufLength) {
    uint8_t bmRequestType = setup[0];
    int bfDataDirection = bmRequestType & 0x80;
    if (bfDataDirection) {
	/* OutRequest */
	return OutRequest(setup, data, replyBuffer, bufLength);
    } else {
	/* InRequest */
	return InRequest(setup, data, replyBuffer, bufLength);
    }

    return EP_STALL;
}

int UsbInterface::OutRequest(uint8_t* setup, uint8_t* data, uint8_t* replyBuffer, int bufLength) {
    uint8_t bRequest = setup[1];
    INFO("UsbInterface: OutRequest %.2x", bRequest);

    if (bRequest == 0x06) {
	return GetDescriptor(setup, data, replyBuffer, bufLength);
    }
    return EP_STALL;
}

int UsbInterface::GetDescriptor(uint8_t* setup, uint8_t* data, uint8_t* replyBuffer, int bufLength) {
    (void)setup;
    (void)data;
    (void)replyBuffer;
    (void)bufLength;
    ERROR("Unkown GetDescriptor UsbInterface"); 
    return EP_STALL;
}

int UsbInterface::InRequest(uint8_t* setup, uint8_t* data, uint8_t* replyBuffer, int bufLength) {
    (void)data;
    (void)replyBuffer;
    (void)bufLength;
    ERROR_VECTOR("Unkown InRequest UsbInterface", setup, 8);
    return EP_STALL;
}

UsbEndpoint* UsbInterface::GetEndpoint(uint8_t endpointAddress) {
    for (int idx = 0; idx < bNumEndpoints; idx++) {
	if ((endpointArray[idx]->bEndpointAddress & 0x7f) == endpointAddress) {
	    return endpointArray[idx];
	}
    }
    return NULL;
}
