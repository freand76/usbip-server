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

#include "UsbString.h"
#include "UsbDevice.h"
#include "UsbUtil.h"

using namespace UsbUtil;

int UsbString::GetStringDescriptor(uint8_t bDescriptorIndex, uint8_t* buffer, int offset) {
    int pos = offset;
    if (bDescriptorIndex == 0) {
	pos += SetUint(3, buffer, pos, 1);
    }

    return EP_STALL;
}
